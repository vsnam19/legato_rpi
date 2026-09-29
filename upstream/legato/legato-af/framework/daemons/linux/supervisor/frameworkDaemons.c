//--------------------------------------------------------------------------------------------------
/** @file frameworkDaemons.c
 *
 * API for managing Legato framework daemons.  The framework daemons include the Service Directory,
 * Log Control, Configuration Tree and Watchdog.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "frameworkDaemons.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "killProc.h"
#include "smack.h"
#include "sysPaths.h"
#include "wait.h"
#include <sys/capability.h>
#include <sys/prctl.h>




//--------------------------------------------------------------------------------------------------
/**
 * The framework daemon object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            path[LIMIT_MAX_PATH_BYTES];     // Path to the daemon's executable.
    int             unneededCapList[LIMIT_MAX_NUM_CAPABILITIES]; // Unneeded capability list.
    size_t          capNum;                         // Number of capabilities in list.
    pid_t           pid;                            // The daemon's pid.
}
DaemonObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Time interval (milliseconds) between when a soft kill and a hard kill happens when shutting down
 * framework daemons.
 */
//--------------------------------------------------------------------------------------------------
#define KILL_TIMEOUT            1000


//--------------------------------------------------------------------------------------------------
/**
 * List of all framework daemons in the order that they must start.
 *
 * @warning The order of the entire list is important and should not be changed without careful
 *          consideration.
 *
 * - The Service Directory must be the first framework daemon in this list.  Everything else needs
 *   it for IPC.
 *
 * - The Log Control Daemon is second because everything else uses logging.
 *
 * - The Config Tree must start before the Update Daemon, because the Update Daemon needs to use the
 *   configuration tree.  Furthermore, the Update Daemon MUST have a chance to update the system
 *   configuration data before anything else that uses that data starts.  This is because the
 *   Update Daemon may need to finish a system update.
 *
 * - The Watchdog Daemon fetches watchdog settings from the system configuration tree.
 */
//--------------------------------------------------------------------------------------------------

static DaemonObj_t FrameworkDaemons[] = {
    {SYSTEM_BIN_PATH "/serviceDirectory", {-1}, 0, -1},
    {SYSTEM_BIN_PATH "/logCtrlDaemon", {CAP_BLOCK_SUSPEND}, 1, -1},
    {SYSTEM_BIN_PATH "/configTree", {-1}, 0, -1},
    {SYSTEM_BIN_PATH "/updateDaemon", {-1}, 0, -1},
#if ! defined (LE_CONFIG_FLAVOR_LXC)
    {SYSTEM_BIN_PATH "/deviceManager", {-1}, 0, -1},
#endif
    {SYSTEM_BIN_PATH "/watchdog", {CAP_BLOCK_SUSPEND}, 1, -1}};


//--------------------------------------------------------------------------------------------------
/**
 * Index in the list of FrameworkDaemons to shutdown.
 */
//--------------------------------------------------------------------------------------------------
static int ShutdownIndex = -1;


//--------------------------------------------------------------------------------------------------
/**
 * The intermediate shutdown notification handler.  This will be called when all framework daemons,
 * except the Service Directory, have shutdown.
 */
//--------------------------------------------------------------------------------------------------
static fwDaemons_ShutdownHandler_t IntermediateShutdownHandler = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The shutdown notification handler.  This will be called when all framework daemons have shutdown.
 */
//--------------------------------------------------------------------------------------------------
static fwDaemons_ShutdownHandler_t ShutdownHandler = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *
 * Configure capabilities according to the adef file settings
 *
 **/
//--------------------------------------------------------------------------------------------------
static void RemoveUnneededCapabilities
(
    const int* capListPtr,
    size_t numOfCaps
)
{
    if (numOfCaps <= 0)
    {
        return;
    }

    if ((numOfCaps > LIMIT_MAX_NUM_CAPABILITIES) || (capListPtr == NULL))
    {
        LE_ERROR("Invalid parameter.");
        return;
    }

    struct __user_cap_data_struct data[2] = {{0}, {0}};
    struct __user_cap_header_struct head = {0};
    head.version = _LINUX_CAPABILITY_VERSION_3;
    head.pid = 0;
    int i, idx;
    uint32_t bit = 0;

    // Get the original capabilities of current process.
    if (capget(&head, data) == -1)
    {
        LE_ERROR("Failed to get capabilities.");
        return;
    }

    // Remove given capabilities from effective,permitted,inheritable sets.
    for (i = 0; i < numOfCaps; i++)
    {
        idx = capListPtr[i]/32;
        bit = 1<<(capListPtr[i]%32);
        data[idx].effective &= ~bit;
        data[idx].permitted &= ~bit;
        data[idx].inheritable &= ~bit;
    }

    if (capset(&head, data) == -1)
    {
        LE_ERROR("Failed to set capabilities.");
        return;
    }

    // Remove given capabilities from Ambient set.
    for (i = 0; i < numOfCaps; i++)
    {
        if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_LOWER, capListPtr[i], 0, 0) == -1)
        {
            LE_ERROR("prctl(PR_CAP_AMBIENT_CLEAR_FLAG) failed for cap(%d).", capListPtr[i]);
            continue;
        }
    }

    // Remove given capabilities from Bounding set.
    for (i = 0; i < numOfCaps; i++)
    {
        if (prctl(PR_CAPBSET_DROP, capListPtr[i]) == -1)
        {
            LE_ERROR("prctl(PR_CAPBSET_DROP) failed for cap(%d).", capListPtr[i]);
            continue;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Load the current IPC binding configuration into the Service Directory.
 **/
//--------------------------------------------------------------------------------------------------
static void LoadIpcBindingConfig
(
    void
)
{

    // Fork a process.
    pid_t pid = fork();
    LE_FATAL_IF(pid < 0, "Failed to fork child process.  %m.");

    if (pid == 0)
    {
        // Launch the child program.  This should not return unless there was an error.
        execlp("sdir", "sdir", "load", (char*)NULL);
        // The program could not be started.
        LE_FATAL("'sdir' could not be started: %m");
    }

    int status;
    pid_t p;

    do
    {
        p = waitpid(pid, &status, 0);
    }
    while((p == -1) && (errno == EINTR));

    if (p != pid)
    {
        if (p == -1)
        {
            LE_FATAL("waitpid() failed: %m");
        }
        else
        {
            LE_FATAL("waitpid() returned unexpected result %d", p);
        }
    }

    if (WIFSIGNALED(status))
    {
        LE_FATAL("Couldn't load IPC binding config. `sdir load` received signal: %d.",
            WTERMSIG(status));
    }
    else if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) != EXIT_SUCCESS)
        {
            LE_FATAL("Couldn't load IPC binding config. `sdir load` exited with code: %d.",
                WEXITSTATUS(status));
        }
    }
    else
    {
        LE_FATAL("Couldn't load IPC binding config. `sdir load` failed for an unknown reason (status = %d).",
            status);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for SIGCHLD while framework daemons are starting up. If one exits or traps, that means
 * that the Legato system is unworkable. Exit from supervisor to trigger a reboot.
 */
//--------------------------------------------------------------------------------------------------
static void SigChldStartingHandler
(
    int sigNum,
    siginfo_t *sigInfoPtr,
    void *dummyPtr
)
{
    // Parameter is not used.
    (void)dummyPtr;
    if (SIGCHLD != sigNum)
    {
        return;
    }

    int i;
    // Loop on all framework system processes. If one has exited or was killed, the Legato system
    // is not workable. Exiting with LE_FATAL() will trigger a reboot from startSystem process
    // after some reboots, trigger a roll-back or a swap (dual-systems).
    for (i = 0; i < NUM_ARRAY_MEMBERS(FrameworkDaemons); i++)
    {
        if ((-1 != FrameworkDaemons[i].pid) && (sigInfoPtr->si_pid == FrameworkDaemons[i].pid))
        {
            LE_CRIT("System process '%s' raising SIGCHLD", FrameworkDaemons[i].path);
            if (CLD_EXITED == sigInfoPtr->si_code)
            {
                LE_CRIT("System process has exited with status %d", (sigInfoPtr->si_status));
            }
            else
            {
                LE_CRIT("System process has been terminated with si_code: %d",
                        sigInfoPtr->si_code);
            }
            LE_FATAL("**** SYSTEM IS UNWORKABLE ****");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a framework daemon.
 */
//--------------------------------------------------------------------------------------------------
static void StartDaemon
(
    DaemonObj_t* daemonPtr      ///< [IN] The daemon to start.
)
{
    LE_FATAL_IF(daemonPtr == NULL, "Invalid daemonPtr.");
    const char* daemonNamePtr = le_path_GetBasenamePtr(daemonPtr->path, "/");

    // Create a synchronization pipe.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) != 0, "Could not create synchronization pipe.  %m.");

    // Fork a process.
    pid_t pid = fork();
    LE_FATAL_IF(pid < 0, "Failed to fork child process.  %m.");

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        sigset_t sigSet;
        LE_ASSERT(sigfillset(&sigSet) == 0);
        LE_ASSERT(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) == 0);

        // The child does not need the read end of the pipe so close it.
        fd_Close(syncPipeFd[0]);

        // Duplicate the write end of the pipe on standard in so the execed program will know
        // where it is.
        if (syncPipeFd[1] != STDIN_FILENO)
        {
            int r;
            do
            {
                r = dup2(syncPipeFd[1], STDIN_FILENO);
            }
            while ( (r == -1)  && (errno == EINTR) );

            LE_FATAL_IF(r == -1, "Failed to duplicate fd.  %m.");

            // Close the duplicate fd.
            fd_Close(syncPipeFd[1]);
        }

        // Close all non-standard fds.
        fd_CloseAllNonStd();

        // Remove unneeded capabilities.
        RemoveUnneededCapabilities(daemonPtr->unneededCapList, daemonPtr->capNum);

        // Launch the child program.  This should not return unless there was an error.
        execl(daemonPtr->path, daemonNamePtr, (char*)NULL);

        // The program could not be started.
        LE_FATAL("'%s' could not be started: %m", daemonPtr->path);
    }

    // Store the pid of the running daemon process.
    daemonPtr->pid = pid;

    // Close the write end of the pipe because the parent does not need it.
    fd_Close(syncPipeFd[1]);

    // Wait for the child process to close the read end of the pipe.  This ensures that the
    // framework daemons start in the proper order.
    // TODO: Add a timeout here.
    ssize_t numBytesRead;
    int dummyBuf;
    do
    {
        numBytesRead = read(syncPipeFd[0], &dummyBuf, 1);
    }
    while ( ((numBytesRead == -1) && (errno == EINTR)) || (numBytesRead != 0) );

    LE_FATAL_IF(numBytesRead == -1, "Could not read synchronization pipe.  %m.");

    // Close the read end of the pipe because it is no longer used.
    fd_Close(syncPipeFd[0]);

    LE_INFO("Started system process '%s' with PID: %d.", daemonNamePtr, pid);
}

//--------------------------------------------------------------------------------------------------
/**
 * Iterate over the process PID and query if it is an orphaned process
 * If so then kill it
 * If not then keep it
 */
//--------------------------------------------------------------------------------------------------
void KillOrphanProcess(const char *pids, const char *procName)
{
    char *saveRestPtr = NULL;
    char *ptoken = strtok_r((char *)pids, " ", &saveRestPtr);

    while (ptoken != NULL)
    {
        int pid = atoi(ptoken);

        char GetPPidCmd[LIMIT_MAX_PATH_BYTES];
        int s = snprintf(GetPPidCmd, sizeof(GetPPidCmd), "cat /proc/%d/stat | cut -d ' ' -f4", pid);

        if (s >= sizeof(GetPPidCmd))
        {
            LE_FATAL("Could not create combined cmd, buffer is too small %zd < %d", sizeof(GetPPidCmd), s);
        }

        FILE *fp = popen(GetPPidCmd, "r");
        if (!fp)
        {
            LE_FATAL("Could not create pipe for getting ppid. [%s]", GetPPidCmd);
        }

        // It's enough for one PID
        char ppidStr[LIMIT_MAX_ARGS_STR_BYTES];
        while (fgets(ppidStr, sizeof(ppidStr), fp) != NULL)
        {
            int ppid = atoi(ppidStr);

            // Only kill orphaned processes and keep non-orphaned processes,
            // because there may be containers running another TelAF framework on the system
            if (1 == ppid)
            {
                LE_INFO("Kill the Orphan process [%s]/(pid:%d).", procName, pid);
                kill_ByName(procName);
            }
            else
            {
                LE_INFO("Keep [%s]/(pid:%d) process here.", procName, pid);
            }
        }

        pclose(fp);

        ptoken = strtok_r(NULL, " ", &saveRestPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the PID of the process by name and check if they are orphaned processes.
 */
//--------------------------------------------------------------------------------------------------
void CheckOrphanProcess(const char *procName)
{
    char GetPidCmd[LIMIT_MAX_PATH_BYTES];
    int len = snprintf(GetPidCmd, sizeof(GetPidCmd), "pidof %s", procName);

    if (len >= sizeof(GetPidCmd))
    {
        LE_FATAL("Could not create 'pidof' cmd, buffer is too small. "
                 "Buffer is %zd bytes but needs to be %d bytes.", sizeof(GetPidCmd), len);
    }

    FILE *fp = popen(GetPidCmd, "r");
    if (!fp)
    {
        LE_FATAL("Could not create pipe for getting pid by name. [%s]", GetPidCmd);
    }

    char pids[LIMIT_MAX_ARGS_STR_BYTES];
    while (fgets(pids, sizeof(pids), fp) != NULL)
    {
        pids[strcspn(pids, "\n")] = '\0';
        KillOrphanProcess(pids, procName);
    }

     pclose(fp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start all the framework daemons.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_Start
(
    void
)
{
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FrameworkDaemons); i++)
    {
        char* daemonNamePtr = le_path_GetBasenamePtr(FrameworkDaemons[i].path, "/");

        // Check the daemon procces, if they are orphan processes, kill them
        // Ohterwise, keep them there
        CheckOrphanProcess(daemonNamePtr);
    }

    int rc;
    struct sigaction sa, so;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT | SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = SigChldStartingHandler;
    rc = sigaction(SIGCHLD, &sa, &so);
    if (rc)
    {
        LE_FATAL("Unable to install SIGCHLD handler: %m");
    }

    for (i = 0; i < NUM_ARRAY_MEMBERS(FrameworkDaemons); i++)
    {
        StartDaemon(&(FrameworkDaemons[i]));
    }

    LE_INFO("All framework daemons ready.");

    // Restore old SIGCHLD handler
    rc = sigaction(SIGCHLD, &so, NULL);
    if (rc)
    {
        LE_FATAL("Unable to restore SIGCHLD handler: %m");
    }

    // Load the current IPC binding configuration into the Service Directory.
    LoadIpcBindingConfig();
}


//--------------------------------------------------------------------------------------------------
/**
 * Shuts down the next running framework daemon starting from the daemonIndex.  This function
 * searches backwards through the list of framework daemons, starting at daemonIndex, for the next
 * running daemon and shuts it down.
 *
 * @note
 *      The shutdown is asynchronous.  When the process actually dies a SIGCHILD will be received.
 *
 * @return
 *      The index of the daemon that we are shutting down.
 *      -1 if all daemons have already died.
 */
//--------------------------------------------------------------------------------------------------
static int ShutdownNextDaemon
(
    int daemonIndex         ///< [IN] Index of the daemon to start with.
)
{
    // Search backwards through the list of daemons to find the last system process that needs to
    // be killed.
    while (daemonIndex >= 0)
    {
        if (FrameworkDaemons[daemonIndex].pid != -1)
        {
            break;
        }

        daemonIndex--;
    }

    if (daemonIndex < 0)
    {
        // All framework daemons already shutdown.
        // Call registered shutdown handler.
        if (ShutdownHandler != NULL)
        {
            ShutdownHandler();
        }
    }
    else
    {
        if (daemonIndex == 0)
        {
            // All framework daemons except the Service Directory has shutdown.  Call the
            // intermediate shutdown handler.
            if (IntermediateShutdownHandler != NULL)
            {
                IntermediateShutdownHandler();
            }
        }

        // Kill the current daemon.
        LE_INFO("Killing framework daemon '%s'.",
                le_path_GetBasenamePtr(FrameworkDaemons[daemonIndex].path, "/"));
        kill_Soft(FrameworkDaemons[daemonIndex].pid, KILL_TIMEOUT);
    }

    return daemonIndex;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiates the shut down of all the framework daemons.  The shut down sequence happens
 * asynchronously.  A shut down handler should be set using fwDaemons_SetShutdownHandler() to be
 * notified when all framework daemons actually shut down.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_Shutdown
(
    void
)
{
    // Set the Shutdown index to the last daemon in the list.
    ShutdownIndex = NUM_ARRAY_MEMBERS(FrameworkDaemons) - 1;

    // Start the shutdown sequence.  After the first framework daemon is shutdown the shutdown
    // sequence will be continued by the fwDaemons_SigChildHandler().
    ShutdownIndex = ShutdownNextDaemon(ShutdownIndex);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the shutdown handler to be called when all the framework daemons shutdown.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_SetShutdownHandler
(
    fwDaemons_ShutdownHandler_t shutdownHandler
)
{
    ShutdownHandler = shutdownHandler;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the intermediate shutdown handler to be called when all the framework daemons shutdown
 * except for the Service Directory.  This gives the caller a chance to do some message handling
 * before the Service Directory is shutdown as well.
 *
 * @note The Service Directory is the last framework daemon to shutdown.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_SetIntermediateShutdownHandler
(
    fwDaemons_ShutdownHandler_t shutdownHandler
)
{
    IntermediateShutdownHandler = shutdownHandler;
}


//--------------------------------------------------------------------------------------------------
/**
 * The SIGCHLD handler for the framework daemons.  This should be called from the Supervisor's
 * SIGCHILD handler.
 *
 * @note
 *      This function will reap the child if the child is a framework daemon, otherwise the child
 *      will remain unreaped.
 *
 * @return
 *      LE_OK if the signal was handled without incident.
 *      LE_NOT_FOUND if the pid is not a framework daemon.  The child will not be reaped.
 *      LE_FAULT if the signal indicates the failure of one of the framework daemons.
 */
//--------------------------------------------------------------------------------------------------
le_result_t fwDaemons_SigChildHandler
(
    pid_t pid               ///< [IN] Pid of the process that produced the SIGCHLD.
)
{
    // See which daemon produced this signal.
    DaemonObj_t* daemonObjPtr = NULL;

    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FrameworkDaemons); i++)
    {
        if (FrameworkDaemons[i].pid == pid)
        {
            daemonObjPtr = &(FrameworkDaemons[i]);

            // Mark this daemon as dead.
            daemonObjPtr->pid = -1;
            kill_Died(pid);

            break;
        }
    }

    if (daemonObjPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // This child process is a framework daemon.
    // Reap the child now.
    int status = wait_ReapChild(pid);

    if (ShutdownIndex >= 0)
    {
        // We are in the midst of a shutdown sequence, continue the shutdown sequence.
        ShutdownIndex = ShutdownNextDaemon(ShutdownIndex);

        return LE_OK;
    }
    else
    {
        const char* daemonName = le_path_GetBasenamePtr(daemonObjPtr->path, "/");

        if (WIFEXITED(status))
        {
            // This was an unexpected error from one of the framework daemons.
            LE_EMERG("Framework daemon '%s' has exited with code %d.",
                     daemonName,
                     WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            // This was an unexpected error from one of the framework daemons.
            LE_EMERG("Framework daemon '%s' has been killed by a signal: %d.",
                     daemonName,
                     WTERMSIG(status));
        }
        else
        {
            // This was an unexpected error from one of the framework daemons.
            LE_EMERG("Framework daemon '%s' has died for an unknown reason (status = 0x%x).",
                     daemonName,
                     status);
        }

        return LE_FAULT;
    }
}
