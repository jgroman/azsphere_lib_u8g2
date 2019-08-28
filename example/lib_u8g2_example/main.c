#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/i2c.h>

// Import project hardware abstraction from project 
// property "Target Hardware Definition Directory"
#include <hw/project_hardware.h>

#include "lib_u8g2.h"

// Support functions.
static void TerminationHandler(int signalNumber);
static int InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
static int i2cFd = -1;

// Termination state
static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	terminationRequired = true;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	// Init I2C
	i2cFd = I2CMaster_Open(PROJECT_ISU2_I2C);
	if (i2cFd < 0) 
    {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", 
            errno, strerror(errno));
		return -1;
	}

	int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (result != 0) 
    {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", 
            errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetTimeout(i2cFd, 100);
	if (result != 0) 
    {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", 
            errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetDefaultTargetAddress(i2cFd, 0x3C);
	if (result != 0) 
    {
		Log_Debug("ERROR: I2CMaster_SetDefaultTargetAddress: errno=%d (%s)\n", 
            errno, strerror(errno));
		return -1;
	}

	return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
	close(i2cFd);
}

/// <summary>
///     Display drawing demo
/// </summary>
void u8x8Main(void)
{
	const struct timespec sleepTime = { 1, 0 };
	u8x8_t u8x8;

    nanosleep(&sleepTime, NULL);

	Log_Debug("Calling Setup\n");
	u8x8_Setup(&u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c, 
        u8x8_byte_i2c, lib_u8g2_custom_cb);
	u8x8_SetI2CAddress(&u8x8, 0x3C);

	Log_Debug("Calling Init\n");
	u8x8_InitDisplay(&u8x8);

	u8x8_SetPowerSave(&u8x8, 0);

	Log_Debug("Calling ClearDisplay\n");
	u8x8_ClearDisplay(&u8x8);

	Log_Debug("Calling Set Font\n");
	u8x8_SetFont(&u8x8, u8x8_font_amstrad_cpc_extended_f);

	Log_Debug("Sending drawstring\n");

	char buff[10];
	uint8_t i;
	uint8_t j = 0;

	for (i = 0; i < 8; i++) 
    {
		for (j = 0; j < 16; j++) 
        {
			sprintf(buff, "%c", j + 65);
			u8x8_DrawString(&u8x8, j, i, buff);
		}
	}

}

/// <summary>
///     Application entry point
/// </summary>
int main(void) {
	Log_Debug("\n*** Starting ***\n");

	if (InitPeripheralsAndHandlers() != 0) {
		terminationRequired = true;
	}

    lib_u8g2_set_i2c_fd(i2cFd);

	if (!terminationRequired) {
		u8x8Main();
	}

	ClosePeripheralsAndHandlers();
	return 0;
}
