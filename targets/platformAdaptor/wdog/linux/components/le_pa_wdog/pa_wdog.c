//--------------------------------------------------------------------------------------------------
/**
 * @file pa_wdog.c
 *
 * Linux Platform Adapter for Legato Watchdog on Raspberry Pi 5.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_wdog.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

static int WdogFd = -1;

void pa_wdog_Init(void)
{
    WdogFd = open("/dev/watchdog", O_WRONLY);
    if (WdogFd >= 0)
    {
        LE_INFO("Hardware watchdog /dev/watchdog opened successfully");
    }
    else
    {
        LE_INFO("Hardware watchdog not present or inaccessible, running in software mode");
    }
}

void pa_wdog_Kick(void)
{
    if (WdogFd >= 0)
    {
        int dummy = 0;
        ioctl(WdogFd, WDIOC_KEEPALIVE, &dummy);
    }
}

void pa_wdog_Shutdown(void)
{
    if (WdogFd >= 0)
    {
        // Magic close character 'V' before closing disables the watchdog gracefully
        write(WdogFd, "V", 1);
        close(WdogFd);
        WdogFd = -1;
    }
}
