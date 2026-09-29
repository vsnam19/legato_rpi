/** @file log.c
 *
 * This module handles the building of the log messages and sending the messages to the log file.
 * Configuration of log messages is also handled by this module.  Writing traces to the log and
 * enabling traces by keyword is also handled here.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "limit.h"
#include "log.h"
#include "logDaemon/logDaemon.h"
#include "logPlatform.h"
#include "messagingSession.h"

#ifdef LE_CONFIG_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>

//--------------------------------------------------------------------------------------------------
/**
 * Data struct of TelAF DLT session
 */
//--------------------------------------------------------------------------------------------------
typedef struct dlt_log_Session
{
    DltContext ctxHandle;                          /// TelAF context handler.
    char appId[DLT_ID_SIZE + 1];                   /// TelAF application ID.
    char ctxId[DLT_ID_SIZE + 1];                   /// TelAF context ID.
    char appDesc[LIMIT_MAX_APP_NAME_BYTES];        /// TelAF application description.
    char ctxDesc[LIMIT_MAX_COMPONENT_NAME_BYTES];  /// TelAF context description.
    DltLogLevelType logLevel;                      /// TelAF DLT log level.
    bool initialized;                              /// TelAF session state.
}
DLTSession_t;

static DLTSession_t DltSession;

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of log messages.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MSG_SIZE            256


//--------------------------------------------------------------------------------------------------
/**
 * Log session.  Stores log configuration for each registered component.  The component names and
 * filters are created and stored by the components' themselves but no one should be accesssing
 * them but this le_log module.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_log_Session
{
    char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];  ///< The component name.
    le_log_Level_t level;               ///< The component's severity level filter.
                                        ///  Log messages with severity less than this are ignored.
    le_sls_List_t keywordList;          ///< The list of keywords for this component.
    le_sls_Link_t link;                 ///< The link used for linking with the SessionList.
}
LogSession_t;


//--------------------------------------------------------------------------------------------------
/**
 * A list of log sessions, one for each component.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t SessionList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * A memory pool for the log sessions.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionMemPool;


//--------------------------------------------------------------------------------------------------
/**
 * Default log session to be used when logging from code that is extremely early in the start-up
 * sequence or that didn't execute the proper component start-up sequence.
 **/
//--------------------------------------------------------------------------------------------------
static LogSession_t DefaultLogSession =    {
                                            .componentName="<invalid>",
                                            .level=LOG_DEFAULT_LOG_FILTER,
                                            .keywordList=LE_SLS_LIST_INIT,
                                            .link=LE_SLS_LINK_INIT
                                        };


//--------------------------------------------------------------------------------------------------
/**
 * A keyword object that contains the keyword string and can be attached to the keyword list.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t link;                        // The link in the keyword list.
    char keyword[LIMIT_MAX_LOG_KEYWORD_BYTES]; // The keyword.
    bool isEnabled;                            // true if the keyword is enabled.  false otherwise.
}
KeywordObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * A memory pool where we get the memory for the keyword objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t KeywordMemPool;


//--------------------------------------------------------------------------------------------------
/**
 * c_messaging Session Reference used to communicate with the Log Control Daemon.
 * NULL if the Log Control Daemon is not available.
 **/
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t IpcSessionRef;


//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 **/
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)


//--------------------------------------------------------------------------------------------------
/**
 * POSIX threads "Fast" mutex used to protect structures in this module from multi-threaded
 * race conditions.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


//--------------------------------------------------------------------------------------------------
/**
 * Default log session and log filter level, for when outside a component.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_log_SessionRef_t LE_LOG_SESSION = NULL;
LE_SHARED le_log_Level_t* LE_LOG_LEVEL_FILTER_PTR = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Lock the mutex.
 */
//--------------------------------------------------------------------------------------------------
static inline void Lock
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Unlock the mutex.
 */
//--------------------------------------------------------------------------------------------------
static inline void Unlock
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Keyword Object for a given session.
 *
 * @warning Assumes that the mutex is locked.
 *
 * @return
 **/
//--------------------------------------------------------------------------------------------------
static KeywordObj_t* CreateKeyword
(
    LogSession_t* logSessionPtr,
    const char* keyword
)
//--------------------------------------------------------------------------------------------------
{
    // The keyword does not exist so we should create it from the memory pool.
    KeywordObj_t* keywordObjPtr = le_mem_ForceAlloc(KeywordMemPool);

    // Copy the keyword into the keyword object.
    le_result_t result = le_utf8_Copy(keywordObjPtr->keyword,
                                      keyword,
                                      sizeof(keywordObjPtr->keyword),
                                      NULL);
    LE_WARN_IF(result == LE_OVERFLOW,
               "Keyword '%s' is truncated to '%s'",
               keyword,
               keywordObjPtr->keyword);

    // Init the keyword object.
    keywordObjPtr->isEnabled = false;
    keywordObjPtr->link = LE_SLS_LINK_INIT;

    // Add the object to the list of keywords.
    le_sls_Queue(&(logSessionPtr->keywordList), &(keywordObjPtr->link));

    return keywordObjPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the keyword object by keyword.
 *
 * @return A pointer to the Keyword Object or NULL if not found.
 *
 * @warning Assumes that mutex is already locked by the caller.
 */
//--------------------------------------------------------------------------------------------------
static KeywordObj_t* GetKeywordObj
(
    const char* keywordPtr,         // The keyword to search for.
    le_sls_List_t* keywordList      // The keyword list to search in.
)
{
    // Search the keyword list for the keyword.
    le_sls_Link_t* keywordLinkPtr = le_sls_Peek(keywordList);

    while (keywordLinkPtr)
    {
        // Get the keyword object.
        KeywordObj_t* keywordObjPtr = CONTAINER_OF(keywordLinkPtr, KeywordObj_t, link);

        if (strcmp(keywordObjPtr->keyword, keywordPtr) == 0)
        {
            return keywordObjPtr;
        }
        keywordLinkPtr = le_sls_PeekNext(keywordList, keywordLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a session by component name.
 *
 * @return A pointer to the Session object or NULL if not found.
 *
 * @warning Assumes that the mutex is held by the caller.
 */
//--------------------------------------------------------------------------------------------------
static LogSession_t* GetSession
(
    const char* componentNamePtr
)
{
    // Find the component's session.
    le_sls_Link_t* sessionLinkPtr = le_sls_Peek(&SessionList);

    while (sessionLinkPtr != NULL)
    {
        // Get the session.
        LogSession_t* sessionPtr = CONTAINER_OF(sessionLinkPtr, LogSession_t, link);

        if (strcmp(componentNamePtr, sessionPtr->componentName) == 0)
        {
            // Found the session.
            return sessionPtr;
        }

        sessionLinkPtr = le_sls_PeekNext(&SessionList, sessionLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable a trace keyword.
 */
//--------------------------------------------------------------------------------------------------
static void EnableTrace
(
    const char* componentNamePtr,   // The component that contains the keyword to disable.
    const char* keywordPtr          // The trace keyword to disable.
)
{
    Lock();

    // Find the session for this component.
    LogSession_t* sessionPtr = GetSession(componentNamePtr);

    if (sessionPtr)
    {
        // Search for the keyword.
        KeywordObj_t* keywordObjPtr = GetKeywordObj(keywordPtr, &(sessionPtr->keywordList));

        if (keywordObjPtr == NULL)
        {
            // The keyword does not exist so we should create it from the memory pool.
            keywordObjPtr = CreateKeyword(sessionPtr, keywordPtr);
        }

        // Enable the keyword.
        keywordObjPtr->isEnabled = true;
    }

    Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Disable a trace keyword.
 */
//--------------------------------------------------------------------------------------------------
static void DisableTrace
(
    const char* componentNamePtr,   // The component that contains the keyword to disable.
    const char* keywordPtr          // The trace keyword to disable.
)
{
    Lock();

    // Find the session for this component.
    LogSession_t* sessionPtr = GetSession(componentNamePtr);

    if (sessionPtr)
    {
        // Search the keyword list for the keyword.
        KeywordObj_t* keywordObjPtr = GetKeywordObj(keywordPtr, &(sessionPtr->keywordList));

        if (keywordObjPtr)
        {
            // Disable the keyword.
            keywordObjPtr->isEnabled = false;
        }
    }

    Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the log level filter for a specific component.
 */
//--------------------------------------------------------------------------------------------------
static void SetLogLevelFilter
(
    const char* componentNamePtr,   // The name of the component.
    int levelFilter                 // The filter level.
)
{
    Lock();

    // Find the session to apply the filter to.
    LogSession_t* sessionPtr = GetSession(componentNamePtr);

    if (sessionPtr)
    {
        // Set this component's level.
        sessionPtr->level = levelFilter;
    }

    Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a log session.
 *
 * @return  Pointer to the new log session object.
 **/
//--------------------------------------------------------------------------------------------------
static LogSession_t* CreateSession
(
    const char* componentNamePtr    // A pointer to the component's name.
)
//--------------------------------------------------------------------------------------------------
{
    LogSession_t* logSessionPtr = le_mem_ForceAlloc(SessionMemPool);

    // Initialize the log session.
    le_utf8_Copy(logSessionPtr->componentName, componentNamePtr,
                 sizeof(logSessionPtr->componentName), NULL);
    logSessionPtr->level = DefaultLogSession.level;
    logSessionPtr->keywordList = LE_SLS_LIST_INIT;
    logSessionPtr->link = LE_SLS_LINK_INIT;

    Lock();

    // Add it to the list of log sessions.
    le_sls_Queue(&SessionList, &(logSessionPtr->link));

    Unlock();

    return logSessionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Loads the default log filter level from the environment, if present.
 **/
//--------------------------------------------------------------------------------------------------
static void ReadLevelFromEnv
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const char* envStrPtr = getenv("LE_LOG_LEVEL");

    if (envStrPtr != NULL)
    {
        le_log_Level_t level = log_StrToSeverityLevel(envStrPtr);

        if (level != (le_log_Level_t)-1)
        {
            DefaultLogSession.level = level;
        }
        else
        {
            LE_ERROR("LE_LOG_LEVEL environment variable has invalid value '%s'.", envStrPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Loads the default list of enabled trace keywords from the environment, if present.
 **/
//--------------------------------------------------------------------------------------------------
static void ReadTraceKeywordsFromEnv
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const char* envStrPtr = getenv("LE_LOG_KEYWORDS");

    if (envStrPtr != NULL)
    {
        char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];
        char keyword[LIMIT_MAX_LOG_KEYWORD_BYTES];

        const char* subStrPtr = envStrPtr;
        size_t i = 0;

        while (i < 1024)  // Arbitrary limit.
        {
            size_t n;

            // Get the component name sub-string (terminated by '/' separator).
            if (le_utf8_CopyUpToSubStr(componentName, subStrPtr + i, "/", sizeof(componentName), &n)
                != LE_OK)
            {
                LE_ERROR("Component name too long in LE_LOG_KEYWORDS environment variable.");
                return;
            }
            else if (n == 0)
            {
                LE_ERROR("Missing component name in LE_LOG_KEYWORDS environment variable.");
                return;
            }
            else if (subStrPtr[n] != '/')
            {
                LE_ERROR("Environment variable LE_LOG_KEYWORDS contains bad keyword specifier"
                         " (Missing '/' separator).");
                return;
            }

            // Skip past the slash.
            i += n + 1;

            // The keyword is the remainder, up to the end of the string or the next colon.
            if (le_utf8_CopyUpToSubStr(keyword, envStrPtr + i, ":", sizeof(keyword), &n)
                != LE_OK)
            {
                LE_ERROR("Keyword too long in LE_LOG_KEYWORDS environment variable.");
                return;
            }
            else if (n == 0)
            {
                LE_ERROR("Missing keyword after '/' in LE_LOG_KEYWORDS environment variable.");
                return;
            }

            // Enable this keyword.
            LE_INFO("Enabling keyword '%s' for component '%s'.", keyword, componentName);
            EnableTrace(componentName, keyword);

            // If we haven't reached the end of the string,
            if (envStrPtr[i + n] == ':')
            {
                // Advance to the beginning of the next sub-string.
                i += n + 1;     // Skip past the colon.
            }
            else
            {
                // We must have reached the null terminator.
                return;
            }
        }

        LE_ERROR("LE_LOG_KEYWORDS environment variable too long.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a command packet, received from the Log Control Daemon, to get the component name,
 * command code and the command data.  The buffer size for the componentName must be
 * LIMIT_MAX_COMPONENT_NAME_BYTES.
 *
 * @return
 *      true if success.
 *      false if the packet was not formated correctly.
 */
//--------------------------------------------------------------------------------------------------
static bool ParseCmdPacket
(
    const char* cmdPacketPtr,   ///< [IN] The command packet.
    char* cmdPtr,               ///< [OUT] Ptr to where the command code should be copied.
                                ///        Can be NULL if not needed.
    char* componentNamePtr,     ///< [OUT] Pointer buffer to copy the component name into.
                                ///        Can be NULL if not needed.
    const char** cmdDataPtrPtr  ///< [OUT] Set to point to the optional CommandData part at the end.
                                ///        Can be NULL if not needed.
)
{
    const char* packetPtr = cmdPacketPtr;
    size_t numBytes = 0;

    TRACE("Parsing packet '%s'", cmdPacketPtr);

    // Check that the command code is there.
    if (*packetPtr == '\0')
    {
        LE_ERROR("Command byte missing from log command message '%s'.", cmdPacketPtr);
        return false;
    }

    // Get the command code.
    if (cmdPtr)
    {
        *cmdPtr = *packetPtr;
    }
    packetPtr++;

    // Get the component name.
    if (componentNamePtr)
    {
        le_result_t result = le_utf8_CopyUpToSubStr(componentNamePtr,
                                                    packetPtr,
                                                    "/",
                                                    LIMIT_MAX_COMPONENT_NAME_BYTES,
                                                    &numBytes);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to extract component name from log command message '%s' (%s).",
                     cmdPacketPtr,
                     LE_RESULT_TXT(result));
            return false;
        }

        packetPtr += numBytes;
    }
    else
    {
        packetPtr = strchr(packetPtr, '/');

        if (!packetPtr)
        {
            LE_ERROR("No slash after component name in log command message '%s'.", cmdPacketPtr);
            return false;
        }
    }

    // Skip the '/' char.
    if (*packetPtr != '/')
    {
        LE_ERROR("Missing slash in log command message '%s'.", cmdPacketPtr);
        return false;
    }
    packetPtr += 1;
    if (*packetPtr == '\0')
    {
        LE_ERROR("Early terminator in log command message '%s'.", cmdPacketPtr);
        return false;
    }

    // Get the command data.
    if (cmdDataPtrPtr)
    {
        *cmdDataPtrPtr = packetPtr;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes a remote logging command.  This function should be called by the event loop when there
 * is a received log command.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessLogCmd
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr  // not used.
)
{
    char* cmdPacketPtr = le_msg_GetPayloadPtr(msgRef);

    char command;
    char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];
    const char* commandDataPtr;

    // Parse the packet.
    if (ParseCmdPacket(cmdPacketPtr, &command, componentName, &commandDataPtr))
    {
        // Process the data
        switch (command)
        {
            case LOG_CMD_SET_LEVEL:
            {
                int level = log_StrToSeverityLevel(commandDataPtr);

                if (level != -1)
                {
                    SetLogLevelFilter(componentName, level);
                }
                break;
            }

            case LOG_CMD_ENABLE_TRACE:
                EnableTrace(componentName, commandDataPtr);
                break;

            case LOG_CMD_DISABLE_TRACE:
                DisableTrace(componentName, commandDataPtr);
                break;

            default:
                LE_ERROR("Invalid command character '%c'.", command);
                break;
        }
    }
    else
    {
        LE_ERROR("Malformed command packet '%s'.", cmdPacketPtr);
    }

    le_msg_ReleaseMsg(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a local log session with the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
static void RegisterWithLogControlDaemon
(
    LogSession_t* logSessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (IpcSessionRef != NULL)
    {
        TRACE("Registering component '%s' with the Log Control Daemon.",
              logSessionPtr->componentName);

        // Allocate a message
        le_msg_MessageRef_t msgRef = le_msg_CreateMsg(IpcSessionRef);
        char* packetPtr = le_msg_GetPayloadPtr(msgRef);
        size_t packetLength = 0;

        // The first character is the registration command character.
        packetPtr[packetLength++] = LOG_CMD_REG_COMPONENT;

        // Copy in the process name.
        size_t n;
        LE_ASSERT(LE_OK == le_utf8_Copy(packetPtr + packetLength,
                                        le_arg_GetProgramName(),
                                        LOG_MAX_CMD_PACKET_BYTES - packetLength,
                                        &n));
        packetLength = packetLength + n;

        // Copy in the component name.
        n = snprintf(packetPtr + packetLength,
                     LOG_MAX_CMD_PACKET_BYTES - packetLength,
                     "/%s/%d",
                     logSessionPtr->componentName,
                     getpid());
        LE_ASSERT(n > 0);

        TRACE("Sending '%s'", packetPtr);

        // Send the registration command and wait for a response from the Log Control Daemon.
        // We do this synchronously because we want to make sure that we don't queue up any
        // component initialization functions to the Event Loop until after we have received
        // all the log setting updates from the Log Control Daemon.  This ensures that the
        // log settings get applied before the component initialization functions run.
        msgRef = le_msg_RequestSyncResponse(msgRef);

        // The response has no payload.
        if (msgRef == NULL)
        {
            LE_ERROR("Log session registration failed!");
        }
        else
        {
            le_msg_ReleaseMsg(msgRef);
        }
    }
}

#ifdef LE_CONFIG_ENABLE_DLT_LOGGING
//--------------------------------------------------------------------------------------------------
/**
 * Hash algorithm used to compute APPID.
 */
//--------------------------------------------------------------------------------------------------
static const char* BuildAppId
(
    const char* namePtr
)
{
    if (namePtr == NULL)
    {
        return NULL;
    }

    // Initialize the hash value.
    uint32_t hashValue = 0;
    uint32_t cnt = 0;

    // Go through the name string.
    while ((*namePtr) && (cnt <= LIMIT_MAX_PATH_LEN))
    {
        // Update the hash value.
        hashValue = (hashValue * 31 + (*namePtr++)) & 0xFFFFFFFF;

        // Increase the counter.
        cnt++;
    }

    // Define the charset.
    static const char Charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
    static const int Base = sizeof(Charset) - 1;

    // Map the hash to charset.
    static char Result[5];
    for (int i = 0; i < 4; i++)
    {
        Result[i] = Charset[hashValue % Base];
        hashValue /= Base;
    }
    Result[4] = '\0';

    return Result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert DLT log level enum values to strings suitable for message logging.
 *
 * @return Severity string.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetDltSeverityStr
(
    DltLogLevelType dltLevel    ///< Severity level.
)
{
    switch (dltLevel)
    {
        case DLT_LOG_OFF:
            return "DLT_LOG_OFF";

        case DLT_LOG_FATAL:
            return "DLT_LOG_FATAL";

        case DLT_LOG_ERROR:
            return "DLT_LOG_ERROR";

        case DLT_LOG_WARN:
            return "DLT_LOG_WARN";

        case DLT_LOG_INFO:
            return "DLT_LOG_INFO";

        case DLT_LOG_DEBUG:
            return "DLT_LOG_DEBUG";

        case DLT_LOG_VERBOSE:
            return "DLT_LOG_VERBOSE";

        default:
            LE_FATAL("unknown DLT log level (%u).", dltLevel);
            return "unknown";
    }

    return "unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts the DLT log level to LE log level
 *
 * @return
 *      DLT log level.
 */
//--------------------------------------------------------------------------------------------------
static le_log_Level_t ConverDltLevelToLELevel
(
    DltLogLevelType dltLevel
)
{
    switch (dltLevel)
    {
        case DLT_LOG_DEBUG:
        case DLT_LOG_VERBOSE:
            return LE_LOG_DEBUG;

        case DLT_LOG_INFO:
            return LE_LOG_INFO;

        case DLT_LOG_WARN:
            return LE_LOG_WARN;

        case DLT_LOG_ERROR:
            return LE_LOG_ERR;

        case DLT_LOG_OFF:
        case DLT_LOG_FATAL:
            return LE_LOG_EMERG;

        default:
            LE_FATAL("unknown DLT log level (%u).", dltLevel);
            return LE_LOG_INFO;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update LE log level for all log components
 */
//--------------------------------------------------------------------------------------------------
static void UpdateLeLogLevel
(
   le_log_Level_t logLevel
)
{
    le_sls_Link_t* sessionLinkPtr = le_sls_Peek(&SessionList);

    while (sessionLinkPtr != NULL)
    {
        // Get the session.
        LogSession_t* sessionPtr = CONTAINER_OF(sessionLinkPtr, LogSession_t, link);

        LE_EMERG("Update LE comp(%s) log level: '%s' -> '%s'", sessionPtr->componentName,
            log_GetSeverityStr(sessionPtr->level), log_GetSeverityStr(logLevel));

        sessionPtr->level = logLevel;
        sessionLinkPtr = le_sls_PeekNext(&SessionList, sessionLinkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * DLT user log level change callback.
 */
//--------------------------------------------------------------------------------------------------
static void DltLogLevelChangeHandler
(
    char contextId[DLT_ID_SIZE],
    uint8_t logLevel,
    uint8_t traceStatus
)
{
    char ctxid[5] = { 0 };

    memcpy(ctxid, contextId, DLT_ID_SIZE);
    if ((strcmp(ctxid, DltSession.ctxId) != 0) ||
        (logLevel == DltSession.logLevel) ||
        (logLevel > DLT_LOG_VERBOSE))
    {
        return;
    }

    LE_INFO("Update DLT log level: '%s' -> '%s'", GetDltSeverityStr(DltSession.logLevel),
                                                   GetDltSeverityStr(logLevel));

    DltSession.logLevel = logLevel;

    DefaultLogSession.level = ConverDltLevelToLELevel(DltSession.logLevel);

    Lock();

    UpdateLeLogLevel(DefaultLogSession.level);

    Unlock();
}
#endif

#ifdef LEGATO_EMBEDDED

#ifdef LE_CONFIG_ENABLE_DLT_LOGGING
//--------------------------------------------------------------------------------------------------
/**
 * Converts the legato log levels to the DLT log levels.
 *
 * @return
 *      DLT log level.
 */
//--------------------------------------------------------------------------------------------------
static DltLogLevelType ConverToDltLevel
(
    le_log_Level_t legatoLevel
)
{
    switch (legatoLevel)
    {
        case LE_LOG_DEBUG:
            return DLT_LOG_DEBUG;

        case LE_LOG_INFO:
            return DLT_LOG_INFO;

        case LE_LOG_WARN:
            return DLT_LOG_WARN;

        case LE_LOG_ERR:
        case LE_LOG_CRIT:
            return DLT_LOG_ERROR;

        case LE_LOG_EMERG:
            return DLT_LOG_FATAL;

        // LE trace message (legatoLevel:-1) is logged at DLT INFO level.
        default:
            return DLT_LOG_INFO;
    }
}
#else

//--------------------------------------------------------------------------------------------------
/**
 * Converts the legato log levels to the syslog priority levels.
 *
 * @return
 *      Syslog priority level.
 */
//--------------------------------------------------------------------------------------------------
static int ConvertToSyslogLevel
(
    le_log_Level_t legatoLevel
)
{
    switch (legatoLevel)
    {
        case LE_LOG_DEBUG:
            return LOG_DEBUG;

        case LE_LOG_INFO:
            return LOG_INFO;

        case LE_LOG_WARN:
            return LOG_WARNING;

        case LE_LOG_ERR:
            return LOG_ERR;

        case LE_LOG_CRIT:
            return LOG_CRIT;

        default:
            return LOG_EMERG;
    }
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Logging to log system
 */
//--------------------------------------------------------------------------------------------------
static void LogMessage
(
    le_log_Level_t legatoLevel,
    const char* logMessagePtr
)
{
#ifdef LE_CONFIG_ENABLE_DLT_LOGGING
    if (DltSession.initialized)
    {
        DLT_LOG(DltSession.ctxHandle, ConverToDltLevel(legatoLevel), DLT_STRING(logMessagePtr));
    }
#else
    syslog(ConvertToSyslogLevel(legatoLevel), "%s", logMessagePtr);
#endif
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Initialize/Reinitialize DLT logging.
 */
//--------------------------------------------------------------------------------------------------
void log_DltInit
(
    void
)
{
#ifdef LE_CONFIG_ENABLE_DLT_LOGGING

    if (DltSession.initialized)
    {
        DltSession.initialized = false;

        DLT_UNREGISTER_CONTEXT(DltSession.ctxHandle);
        DLT_UNREGISTER_APP_FLUSH_BUFFERED_LOGS();
        dlt_free();
    }

    // Set the default log level from LE log level.
    DltSession.logLevel = ConverToDltLevel(DefaultLogSession.level);

    // Set default DLT appId/ctxId.
    le_utf8_Copy(DltSession.appId, "TAFA", sizeof(DltSession.appId), NULL);
    le_utf8_Copy(DltSession.ctxId, "TAFC", sizeof(DltSession.ctxId), NULL);

    // Set default DLT appDesc/ctxDesc
    le_utf8_Copy(DltSession.appDesc, "TelAF application", sizeof(DltSession.appDesc), NULL);
    le_utf8_Copy(DltSession.ctxDesc, "TelAF Context", sizeof(DltSession.ctxDesc), NULL);

    // Fill the dedicated DLT appId/appDesc according to the calling process name.
    char  procPath[LIMIT_MAX_PATH_BYTES] = { 0 };
    char  appPath[LIMIT_MAX_PATH_BYTES] = { 0 };
    char* appName = NULL;

    if ((snprintf(procPath, sizeof(procPath), "/proc/%d/exe", getpid()) < sizeof(procPath)) &&
        (readlink(procPath, appPath, sizeof(appPath)) > 0))
    {
        // Get the program name from the executable Path.
        appName = le_path_GetBasenamePtr(appPath, "/");

        // Set the APPID and APPDesc.
        le_utf8_Copy(DltSession.appDesc, appName, sizeof(DltSession.appDesc), NULL);
        le_utf8_Copy(DltSession.appId, BuildAppId(appName), sizeof(DltSession.appId), NULL);
    }

    const char* envAppIdPtr = getenv("TAF_DLT_APP_ID");
    const char* envCtxIdPtr = getenv("TAF_DLT_CTX_ID");

    // Override DLT appId/ctxId from envs.
    if (envAppIdPtr != NULL)
    {
        le_utf8_Copy(DltSession.appId, envAppIdPtr, sizeof(DltSession.appId), NULL);
    }
    if (envCtxIdPtr != NULL)
    {
        le_utf8_Copy(DltSession.ctxId, envCtxIdPtr, sizeof(DltSession.ctxId), NULL);
    }

    // Register DLT APPID and CTXID with descriptions.
    DLT_REGISTER_APP(DltSession.appId, DltSession.appDesc);
    dlt_register_context_ll_ts(&DltSession.ctxHandle, DltSession.ctxId, DltSession.ctxDesc,
                               DltSession.logLevel, DLT_TRACE_STATUS_OFF);
    DLT_REGISTER_LOG_LEVEL_CHANGED_CALLBACK(DltSession.ctxHandle, DltLogLevelChangeHandler);

    // DLT logging initialization is done.
    DltSession.initialized = true;

#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the logging system.
 */
//--------------------------------------------------------------------------------------------------
void fa_log_Init
(
    void
)
{
    // NOTE: This is called when there is only one thread running, so no need to lock the mutex.

    // Load the default log level filter and output destination settings from the environment.
    ReadLevelFromEnv();

    // Create the keyword memory pool.
    KeywordMemPool = le_mem_CreatePool("TraceKeys", sizeof(KeywordObj_t));
    le_mem_ExpandPool(KeywordMemPool, 10);   /// @todo Make this configurable.

    // Create the session memory pool.
    SessionMemPool = le_mem_CreatePool("LogSession", sizeof(LogSession_t));
    le_mem_ExpandPool(SessionMemPool, 10);  /// @todo Make this configurable.

    // Register the framework as a component.
    LE_LOG_SESSION = le_log_RegComponent(STRINGIZE(LE_COMPONENT_NAME), &LE_LOG_LEVEL_FILTER_PTR);

    // Load the default list of enabled trace keywords from the environment.
    ReadTraceKeywordsFromEnv();

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("logControl");

#ifdef LE_CONFIG_ENABLE_DLT_LOGGING
    // Initialize DLT session.
    memset(&DltSession, 0, sizeof(DltSession));
    log_DltInit();
#else
    // Otherwise use syslogd as the default log system.
    openlog("TelAF", 0, LOG_USER);
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Re-Initialize the logging system.
 */
//--------------------------------------------------------------------------------------------------
void log_ReInit
(
    void
)
{
    closelog();
    openlog("TelAF", 0, LOG_USER);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connects to the Log Control Daemon.  This must not be done until after the Messaging system
 * is initialized, but should be done as soon as possible.  Anything that gets logged before
 * this is called may get logged with settings that don't match what has been set using the
 * log control tool.
 */
//--------------------------------------------------------------------------------------------------
void le_log_ConnectToControlDaemon
(
    void
)
{
    // NOTE: This is called when there is only one thread running, so no need to lock the mutex.

    // Attempt to open an IPC session with the Log Control Daemon.

    le_msg_ProtocolRef_t protocolRef;
    protocolRef = le_msg_GetProtocolRef(LOG_CONTROL_PROTOCOL_ID, LOG_MAX_CMD_PACKET_BYTES);
    IpcSessionRef = le_msg_CreateSession(protocolRef, LOG_CLIENT_SERVICE_NAME);

    // Note: the process's main thread will always run the log command message receive handler.
    le_msg_SetSessionRecvHandler(IpcSessionRef, ProcessLogCmd, NULL);

    le_result_t result = le_msg_TryOpenSessionSync(IpcSessionRef);
    if (result != LE_OK)
    {
        // If the Log Control Daemon isn't running, we just log a debug message and keep running
        // anyway. This allows the use of liblegato for programs that need to start when the
        // Log Control Daemon isn't running or isn't accessible.  For example, it allows tools
        // like the "config" tool or "sdir" tool to still provide useful output to their user
        // when they are run while the Legato framework is stopped.

        LE_DEBUG("Could not connect to log control daemon.");

        le_msg_DeleteSession(IpcSessionRef);
        IpcSessionRef = NULL;

        switch (result)
        {
            case LE_UNAVAILABLE:
                LE_DEBUG("Service not offered by Log Control Daemon."
                         " Is the Log Control Daemon is not running?");
                break;

            case LE_NOT_PERMITTED:
                LE_DEBUG("Missing binding to log client service.");
                break;

            case LE_COMM_ERROR:
                // A debug message will have already been logged, so don't need to do anything.
                break;

            default:
                LE_CRIT("le_msg_TryOpenSessionSync() returned unexpected result code %d (%s)\n",
                        result,
                        LE_RESULT_TXT(result));
                break;
        }
    }
    else
    {
        // Register everything with the Log Control Daemon
        le_sls_Link_t* linkPtr = le_sls_Peek(&SessionList);
        while (linkPtr != NULL)
        {
            LogSession_t* logSessionPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

            RegisterWithLogControlDaemon(logSessionPtr);

            linkPtr = le_sls_PeekNext(&SessionList, linkPtr);
        }
    }
}

/// Expose old symbol name to support apps compiled against an older liblegato.
__attribute__((deprecated)) void log_ConnectToControlDaemon
(
    void
)
{
    le_log_ConnectToControlDaemon();
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a named component with the logging system.
 *
 * @return
 *      A log session reference.  This reference must be kept by the component and accessible
 *      through a local macro with the name LE_LOG_SESSION.
 */
//--------------------------------------------------------------------------------------------------
le_log_SessionRef_t le_log_RegComponent
(
    const char* componentNamePtr,       ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtrPtr  ///< [OUT] Set to point to the component's level filter.
)
{
    // Create a log session.
    LogSession_t* logSessionPtr = CreateSession(componentNamePtr);

    // If this is not the Log Control Daemon itself, try to register the calling component with
    // the Log Control Daemon.
    if (strcmp(componentNamePtr, "le_logDaemon") != 0)
    {
        RegisterWithLogControlDaemon(logSessionPtr);
    }

    *levelFilterPtrPtr = &logSessionPtr->level;

    // Give the log session back to the caller.
    return logSessionPtr;
}

/// Expose old symbol name to support apps compiled against an older liblegato.
__attribute__((deprecated)) le_log_SessionRef_t log_RegComponent
(
    const char       *componentNamePtr,
    le_log_Level_t  **levelFilterPtrPtr
)
{
    return le_log_RegComponent(componentNamePtr, levelFilterPtrPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Builds the log message and sends it to the logging system.
 */
//--------------------------------------------------------------------------------------------------
void fa_log_Send
(
    const le_log_Level_t     level,             // The severity level. Set to -1 if this is a Trace
                                                // log.
    const le_log_TraceRef_t  traceRef,          // The Trace reference. Set to NULL if this is not a
                                                // Trace log.
    le_log_SessionRef_t      logSession,        // The log session.
    const char              *filenamePtr,       // The name of the source file that logged the
                                                // message.
    const char              *functionNamePtr,   // The name of the function that logged the message.
    const unsigned int       lineNumber,        // The line number in the source file that logged
                                                // the message.
    const char              *formatPtr,         // The user message format.
    va_list                  args               // Positional parameters.
)
{
    // Save the current errno to be used in the log message because some of the system calls below
    // may change errno.
    int savedErrno = errno;

    // If the logging function was called from code that doesn't have a log session reference,
    if (logSession == NULL)
    {
        // Use the default log session.
        logSession = &DefaultLogSession;

        // Check that the message's log level is actually higher than the default filtering
        // level, since the logging macros probably weren't provided with a valid pointer
        // to a filtering level.
        if ((level < logSession->level) && (level != (le_log_Level_t)-1))
        {
            return;
        }
    }

    // Get either the log level or the trace keyword.
    const char* levelPtr;

    if ( (level <= LOG_DEBUG) && (level >= LOG_EMERG) )
    {
        // Use the severity level.
        levelPtr = log_GetSeverityStr(level);
    }
    else
    {
        // NOTE: The reference is actually a pointer to the isEnabled flag inside the
        //       keyword object.
        KeywordObj_t* keywordObjPtr = CONTAINER_OF(traceRef, KeywordObj_t, isEnabled);

        // Add the trace keyword.
        levelPtr = keywordObjPtr->keyword;
    }

    // Get the component name.
    // NOTE: The component name won't change, so it's safe to read this without locking the mutex.
    const char* compNamePtr = logSession->componentName;

    // Get the file name.
    char* baseFileNamePtr = le_path_GetBasenamePtr((char*)filenamePtr, "/");

    // Get the thread name.
    const char* threadNamePtr = le_thread_GetMyName();

    // Get the process name.
    const char* procNamePtr = le_arg_GetProgramName();
    if (procNamePtr == NULL)
    {
        procNamePtr = "n/a";
    }

    // Get the user message.
    char msg[MAX_MSG_SIZE] = "";

    // Reset the errno to ensure that we report the proper errno value.
    errno = savedErrno;

    // Don't need to check the return value because if there is an error we can't do anything about
    // it.  If there was a truncation then that'll just show up in the logs.
    vsnprintf(msg, sizeof(msg), formatPtr, args);

    // If running on an embedded target, write the message out to the log.
#ifdef LEGATO_EMBEDDED

    char rawMsg[MAX_MSG_SIZE*2] = "";
    if (functionNamePtr == NULL)
    {
        snprintf(rawMsg, sizeof(rawMsg), "%s | %s[%d]/%s T=%s | %s %d | %s\n",
            levelPtr, procNamePtr, getpid(), compNamePtr, threadNamePtr, baseFileNamePtr,
            lineNumber, msg);
    }
    else
    {
        snprintf(rawMsg, sizeof(rawMsg), "%s | %s[%d]/%s T=%s | %s %s() %d | %s\n",
            levelPtr, procNamePtr, getpid(), compNamePtr, threadNamePtr, baseFileNamePtr,
            functionNamePtr, lineNumber, msg);
    }
    LogMessage(level, rawMsg);

    // If running on a PC, write the message to standard error with a timestamp added.
#else

    time_t now;
    char timeStamp[26] = "";
    char* timeStampPtr = timeStamp;

    if ( (time(&now) != ((time_t)-1)) && (ctime_r(&now, timeStamp) != NULL) )
    {
        // Tue Jan 14 18:01:56 2014
        // 0123456789012345678901234
        timeStampPtr = timeStamp + 4; // Skip day of week.
        timeStamp[19] = '\0';  // Exclude the year.
    }

    if (functionNamePtr == NULL)
    {
        fprintf(stderr, "%s : %s | %s[%d]/%s T=%s | %s %d | %s\n",
                timeStampPtr, levelPtr, procNamePtr, getpid(), compNamePtr,
                threadNamePtr, baseFileNamePtr, lineNumber, msg);
    }
    else
    {
        fprintf(stderr, "%s : %s | %s[%d]/%s T=%s | %s %s() %d | %s\n",
            timeStampPtr, levelPtr, procNamePtr, getpid(), compNamePtr, threadNamePtr,
            baseFileNamePtr, functionNamePtr, lineNumber, msg);
    }

#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @return  The trace reference.
 **/
//--------------------------------------------------------------------------------------------------
le_log_TraceRef_t fa_log_GetTraceRef
(
    const le_log_SessionRef_t   logSession,     ///< [IN] The log session.
    const char*                 keywordPtr      ///< [IN] Pointer to the keyword string.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(logSession != NULL);

    Lock();

    KeywordObj_t* keywordObjPtr = GetKeywordObj(keywordPtr, &(logSession->keywordList));

    if (keywordObjPtr == NULL)
    {
        keywordObjPtr = CreateKeyword(logSession, keywordPtr);
    }

    Unlock();

    // NOTE: The reference is actually a pointer to the isEnabled flag inside the keyword
    //       object.
    return (le_log_TraceRef_t)(&keywordObjPtr->isEnabled);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for a given log session in the calling process.
 *
 * @note    This does not affect other processes and does not update the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
void fa_log_SetFilterLevel
(
    le_log_SessionRef_t logSession,
    le_log_Level_t level
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(logSession != NULL);
    logSession->level = level;
}


//--------------------------------------------------------------------------------------------------
/**
 * Logs a generic message with the given information.
 */
//--------------------------------------------------------------------------------------------------
void log_LogGenericMsg
(
    le_log_Level_t level,       ///< [IN] Severity level.
    const char* procNamePtr,    ///< [IN] Process name.
    pid_t pid,                  ///< [IN] PID of the process.
    const char* msgPtr          ///< [IN] Message.
)
{
    // Write the message out to the log.
#ifdef LEGATO_EMBEDDED

    char rawMsg[MAX_MSG_SIZE*2] = "";
    snprintf(rawMsg, sizeof(rawMsg), "%s | %s[%d] | %s\n",
             log_GetSeverityStr(level), procNamePtr, pid, msgPtr);
    LogMessage(level, rawMsg);

#else

    time_t now;
    char timeStamp[26] = "";
    char* timeStampPtr = timeStamp;

    if ( (time(&now) != ((time_t)-1)) && (ctime_r(&now, timeStamp) != NULL) )
    {
        // Tue Jan 14 18:01:56 2014
        // 0123456789012345678901234
        timeStampPtr = timeStamp + 4; // Skip day of week.
        timeStamp[19] = '\0';  // Exclude the year.
    }

    fprintf(stderr, "%s : %s | %s[%d] | %s\n",
            timeStampPtr, log_GetSeverityStr(level), procNamePtr, pid, msgPtr);

#endif

}
