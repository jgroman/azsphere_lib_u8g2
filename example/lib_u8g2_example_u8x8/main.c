/***************************************************************************//**
* @file    main.c
* @version 1.0.0
* @authors Jaroslav Groman
*
* @par Project Name
*     Lib_u8g2 example.
*
* @par Description
*    .
*
* @par Target device
*    Azure Sphere MT3620
*
* @par Related hardware
*    Avnet Azure Sphere Starter Kit
*    OLED display 128 x 64
*
* @par Code Tested With
*    1. Silicon: Avnet Azure Sphere Starter Kit
*    2. IDE: Visual Studio 2017
*    3. SDK: Azure Sphere SDK Preview
*
* @par Notes
*    .
*
*******************************************************************************/

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
#include <applibs/gpio.h>

// Import project hardware abstraction from project 
// property "Target Hardware Definition Directory"
#include "../hardware/avnet_mt3620_sk/inc/hw/project_hardware.h"

// Using a single-thread event loop pattern based on Epoll and timerfd
#include "epoll_timerfd_utilities.h"

#include "lib_u8g2.h"

/*******************************************************************************
*   Macros and #define Constants
*******************************************************************************/

#define I2C_ISU             PROJECT_ISU2_I2C
#define I2C_BUS_SPEED       I2C_BUS_SPEED_STANDARD
#define I2C_TIMEOUT_MS      (100u)

#define I2C_ADDR_OLED       (0x3C)

#define OLED_LINE_LENGTH    (16u)

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

/**
 * @brief Application termination handler.
 *
 * Signal handler for termination requests. This handler must be
 * async-signal-safe.
 *
 * @param signal_number
 *
 */
static void
termination_handler(int signal_number);

/**
 * @brief Initialize signal handlers.
 *
 * Set up SIGTERM termination handler.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_handlers(void);

/**
 * @brief Initialize peripherals.
 *
 * Initialize all peripherals used by this project.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_peripherals(void);

/**
 *
 */
static void
close_peripherals_and_handlers(void);

/**
 * @brief Button1 press handler
 */
static void
handle_button1_press(void);

/**
 * @brief Timer event handler for polling button states
 */
static void
event_handler_timer_button(EventData *event_data);


/*******************************************************************************
* Global variables
*******************************************************************************/

// Termination state flag
volatile sig_atomic_t gb_is_termination_requested = false;

static int g_fd_epoll = -1;        // Epoll file descriptor
static int g_fd_i2c = -1;          // I2C interface file descriptor
static int g_fd_gpio_button1 = -1; // GPIO button1 file descriptor
static int g_fd_poll_timer_button = -1;    // Poll timer button press file desc.

static GPIO_Value_Type g_state_button1 = GPIO_Value_High;

static EventData g_event_data_button = {          // Button Event data
    .eventHandler = &event_handler_timer_button
};

static u8x8_t g_u8x8;              // OLED device descriptor

/*******************************************************************************
* Function definitions
*******************************************************************************/

/// <summary>
///     Application entry point
/// </summary>
int 
main(int argc, char *argv[]) 
{
	Log_Debug("\n*** Starting ***\n");
    Log_Debug("Press Button 1 to exit.\n");

    gb_is_termination_requested = false;

    // Initialize handlers
    if (init_handlers() != 0)
    {
        // Failed to init handlers
        gb_is_termination_requested = true;
    }

    // Initialize peripherals
    if (!gb_is_termination_requested)
    {
        if (init_peripherals() != 0)
        {
            // Failed to init peripherals
            gb_is_termination_requested = true;
        }
    }

	if (!gb_is_termination_requested) 
    {
        // All handlers and peripherals are initialized properly at this point

        u8x8_ClearDisplay(&g_u8x8);

        // Basic u8x8 character grid demonstration
        Log_Debug("Calling Set Font\n");
        u8x8_SetFont(&g_u8x8, u8x8_font_amstrad_cpc_extended_f);

        Log_Debug("Sending drawstring\n");

        char buff[30];
        uint8_t i;
        uint8_t j = 0;

        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 16; j++)
            {
                sprintf(buff, "%c", j + (16 * i) + 65);
                u8x8_DrawString(&g_u8x8, j, i, buff);
            }
        }

        // Main program loop
        while (!gb_is_termination_requested)
        {
            // Handle timers
            if (WaitForEventAndCallHandler(g_fd_epoll) != 0)
            {
                gb_is_termination_requested = true;
            }
        }

        u8x8_ClearDisplay(&g_u8x8);
	}

    close_peripherals_and_handlers();

    Log_Debug("*** Terminated ***\n");
    return 0;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static void
termination_handler(int signal_number)
{
    gb_is_termination_requested = true;
}

static int
init_handlers(void)
{
    Log_Debug("Init Handlers\n");

    int result = -1;

    // Create signal handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = termination_handler;
    result = sigaction(SIGTERM, &action, NULL);
    if (result != 0) 
    {
        Log_Debug("ERROR: %s - sigaction: errno=%d (%s)\n",
            __FUNCTION__, errno, strerror(errno));
    }

    // Create epoll
    if (result == 0)
    {
        g_fd_epoll = CreateEpollFd();
        if (g_fd_epoll < 0)
        {
            result = -1;
        }
    }

    return result;
}

static int
init_peripherals(void)
{
    int result = -1;

    // Initialize I2C bus
    Log_Debug("Init I2C\n");
    g_fd_i2c = I2CMaster_Open(I2C_ISU);
    if (g_fd_i2c < 0) 
    {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n",
            errno, strerror(errno));
    }
    else
    {
        result = I2CMaster_SetBusSpeed(g_fd_i2c, I2C_BUS_SPEED);
        if (result != 0)
        {
            Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n",
                errno, strerror(errno));
        }
        else
        {
            result = I2CMaster_SetTimeout(g_fd_i2c, I2C_TIMEOUT_MS);
            if (result != 0) 
            {
                Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n",
                    errno, strerror(errno));
            }
        }
    }

    // Initialize 128x64 SSD1306 OLED
    if (result != -1)
    {
        Log_Debug("Initializing OLED display.\n");

        // Setup u8x8 display type and custom callbacks
        u8x8_Setup(&g_u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c,
            lib_u8g2_byte_i2c, lib_u8g2_custom_cb);

        // Set OLED display I2C interface file descriptor and address
        lib_u8g2_set_i2c(g_fd_i2c, I2C_ADDR_OLED);

        // Initialize display itself
        u8x8_InitDisplay(&g_u8x8);

        // Wake up display
        u8x8_SetPowerSave(&g_u8x8, 0);
    }

    // Initialize development kit button GPIO
    // -- Open button1 GPIO as input
    if (result != -1)
    {
        Log_Debug("Opening PROJECT_BUTTON_1 as input.\n");
        g_fd_gpio_button1 = GPIO_OpenAsInput(PROJECT_BUTTON_1);
        if (g_fd_gpio_button1 < 0) 
        {
            Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n",
                strerror(errno), errno);
            result = -1;
        }
    }

    // Create timer for button press check
    if (result != -1)
    {
        struct timespec button_press_check_period = { 0, 1000000 };

        g_fd_poll_timer_button = CreateTimerFdAndAddToEpoll(g_fd_epoll,
            &button_press_check_period, &g_event_data_button, EPOLLIN);
        if (g_fd_poll_timer_button < 0)
        {
            Log_Debug("ERROR: Could not create button poll timer: %s (%d).\n",
                strerror(errno), errno);
            result = -1;
        }
    }

    return result;
}

static void
close_peripherals_and_handlers(void)
{
    // Close Epoll fd
    CloseFdAndPrintError(g_fd_epoll, "Epoll");

    // Close I2C
    CloseFdAndPrintError(g_fd_i2c, "I2C");

    // Close button1 GPIO fd
    CloseFdAndPrintError(g_fd_gpio_button1, "Button1 GPIO");
}

static void
handle_button1_press(void)
{
    Log_Debug("Button1 pressed.\n");
    gb_is_termination_requested = true;
}

void
event_handler_timer_button(EventData *event_data)
{
    bool b_is_all_ok = true;
    GPIO_Value_Type state_button1_current;

    // Consume timer event
    if (ConsumeTimerFdEvent(g_fd_poll_timer_button) != 0)
    {
        // Failed to consume timer event
        gb_is_termination_requested = true;
        b_is_all_ok = false;
    }

    if (b_is_all_ok)
    {
        // Check for a button1 press
        if (GPIO_GetValue(g_fd_gpio_button1, &state_button1_current) != 0)
        {
            Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n",
                strerror(errno), errno);
            gb_is_termination_requested = true;
            b_is_all_ok = false;
        }
        else if (state_button1_current != g_state_button1)
        {
            if (state_button1_current == GPIO_Value_Low)
            {
                handle_button1_press();
            }
            g_state_button1 = state_button1_current;
        }
    }

    return;
}

/* [] END OF FILE */
