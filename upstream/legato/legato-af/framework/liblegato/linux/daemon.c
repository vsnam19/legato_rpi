//--------------------------------------------------------------------------------------------------
/** @file daemon.c
 *
 * Implementation of process daemonization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "daemon.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Redirect stderr to file specified in parameter.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RedirectStderr
(
    const char* filePathPtr          ///< [IN] File path where stderr should be redirected.
)
{
    int fd;

    do
    {
        fd = open(filePathPtr, O_WRONLY);
    }
    while ((fd == -1) && (errno == EINTR));

    if (fd < 0)
    {
        LE_ERROR("Failed to open '%s'. %m", filePathPtr);
        return LE_FAULT;
    }
    else
    {
        int dupfd;
        do
        {
            dupfd = dup2(fd, fileno(stderr));
        }
        while((dupfd == -1) && (errno == EINTR));

        fd_Close(fd);

        if (dupfd < 0)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Daemonizes the calling process.
 *
 * This function only returns in the child process. In the parent, it waits until the
 * child process its stdin, then terminates itself with a 0 (EXIT_SUCCESS) exit code.
 *
 * The child process should use code like the following to close stdin and reopen it to /dev/null
 * when it wants the parent process to exit.
 *
 * @code
 *
 * LE_FATAL_IF(freopen("/dev/null", "r", stdin) == NULL,
 *             "Failed to redirect stdin to /dev/null.  %m.");
 *
 * @endcode
 *
 * If anything goes wrong, a fatal error is logged and the process exits with EXIT_FAILURE.
 *
 * If the parent times out, it logs a warning message and exits with EXIT_SUCCESS.
 */
//--------------------------------------------------------------------------------------------------
void daemon_Daemonize
(
    uint timeoutMs  ///< Max time to wait for child to close its stdin (in millisecs). -1 = forever.
)
{
    // Create a pipe to use to synchronize the parent and the child.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) != 0, "Could not create synchronization pipe.  %m.");

    // Fork off the parent process.
    pid_t pid = fork();

    LE_FATAL_IF(pid < 0, "Failed to fork when daemonizing the supervisor.  %m.");

    // If we got a non-zero PID, we are the parent process.
    if (pid > 0)
    {
        // The parent does not need the write end of the pipe so close it.
        fd_Close(syncPipeFd[1]);

        // Block until the pipe closes (with timeout).
        struct pollfd waitList;
        waitList.fd = syncPipeFd[0];
        waitList.events = 0;
        waitList.revents = 0;
        int pollResult = poll(&waitList, 1, timeoutMs);
        switch (pollResult)
        {
            case -1:    // Failed.

                LE_FATAL("poll() failed: %m");

            case 0:     // Timeout

                LE_WARN("Timed out after waiting %u ms for indication from child.",
                        timeoutMs);
                exit(EXIT_SUCCESS);

            case 1:     // Child closed their end of the pipe.

                exit(EXIT_SUCCESS);

            default:

                LE_FATAL("Unexpected result (%d) from poll().", pollResult);
        }
    }

    // Only the child gets here.

    // The child does not need the read end of the pipe so close it.
    fd_Close(syncPipeFd[0]);

    // Move the write end of the pipe to stdin, to be closed when the framework is ready for use.
    // Note: This closes stdin and replaces it with the pipe fd.
    while (dup2(syncPipeFd[1], 0) == -1)
    {
        if (errno != EINTR)
        {
            LE_FATAL("dup2(%d, %d) failed: %m", syncPipeFd[1], 0);
        }
    }
    fd_Close(syncPipeFd[1]);

    // Start a new session and become the session leader, the process group leader which will free
    // us from any controlling terminals.
    LE_FATAL_IF(setsid() == -1, "Could not start a new session.  %m.");

    // Reset the file mode mask.
    umask(0);

    // Change the current working directory to the root filesystem, to ensure that it doesn't tie
    // up another filesystem and prevent it from being unmounted.
    LE_FATAL_IF(chdir("/") < 0, "Failed to set working directory to root.  %m.");

    // Redirect stderr to /dev/console will cause the first time serial login failure. Systemd
    // service(serial-getty@ttyMSM0.service) has specified the tty device for /dev/console. Need
    // to get this device from /proc/consoles and for redirecting.
    char *savePtr = NULL;
    char* token = NULL;
    char lineBuf[64] = {'\0'};
    int retBytes;
    char delim[2] = " ";
    char targetConsole[64];
    const char *procConsole = "/proc/consoles";
    const char *defaultConsole = "/dev/console";
    bool isUsingTTY = false;

    int fd = le_fd_Open(procConsole, O_RDONLY);
    if (-1 != fd)
    {
        retBytes = le_fd_Read(fd, lineBuf, sizeof(lineBuf));
        if (retBytes > 0)
        {
            // lineBuf should be null-terminated.
            lineBuf[sizeof(lineBuf) - 1] = '\0';

            token = strtok_r(lineBuf, delim, &savePtr);
            if (token != NULL)
            {
                snprintf (targetConsole, sizeof(targetConsole), "/dev/%s", token);
                isUsingTTY = true;
            }
        }
        le_fd_Close(fd);
    }

    if (isUsingTTY == false)
    {
        le_utf8_Copy(targetConsole, defaultConsole, sizeof(targetConsole), NULL);
    }

    // Redirect stderr to targetConsole. If failed then redirect it to /dev/null. Don't use freopen()
    // function here, as freopen() closes the supplied stream regardless of whether freopen()
    // succeeds (See: http://man7.org/linux/man-pages/man3/freopen.3p.html).
    LE_INFO("use %s to redirect stderr", targetConsole);
    if (RedirectStderr(targetConsole) != LE_OK)
    {
        LE_WARN("Could not redirect stderr to %s (%m), redirecting it to /dev/null.", targetConsole);

        LE_FATAL_IF(RedirectStderr("/dev/null") != LE_OK,
                    "Failed to redirect stderr to /dev/null.  %m.");
    }

    // Redirect stdout to /dev/null.
    LE_FATAL_IF( (freopen("/dev/null", "w", stdout) == NULL),
                "Failed to redirect stdout to /dev/null.  %m.");
}
