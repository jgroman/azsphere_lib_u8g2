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
#include <hw/project_hardware.h>

// Using a single-thread event loop pattern based on Epoll and timerfd
#include "epoll_timerfd_utilities.h"

#include "lib_u8g2.h"
#include "logo.h"

/*******************************************************************************
*   Macros and #define Constants
*******************************************************************************/

#define I2C_ISU             PROJECT_ISU2_I2C
#define I2C_BUS_SPEED       I2C_BUS_SPEED_STANDARD
#define I2C_TIMEOUT_MS      (100u)

#define I2C_ADDR_OLED       (0x3C)

#define OLED_ROTATION       U8G2_R0

typedef enum
{
    SCR_LOGO,
    SCR_FONT,
    SCR_GRAPHICS
} screen_id_t;

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


static void
display_screen(screen_id_t scr_id);


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

static u8g2_t g_u8g2;           // OLED device descriptor for u8g2

static screen_id_t g_screen_id = SCR_LOGO;  // Displayed screen id

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
    Log_Debug("Press Button 1 to move to next screen.\n");

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

        u8g2_ClearDisplay(&g_u8g2);

        // Main program loop
        while (!gb_is_termination_requested)
        {
            // Update OLED display contents
            display_screen(g_screen_id);

            // Handle timers
            if (WaitForEventAndCallHandler(g_fd_epoll) != 0)
            {
                gb_is_termination_requested = true;
            }
        }

        u8g2_ClearDisplay(&g_u8g2);
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
        // Set lib_u8g2 I2C interface file descriptor and device address
        lib_u8g2_set_i2c(g_fd_i2c, I2C_ADDR_OLED);

        // Set display type and callbacks
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(&g_u8g2, OLED_ROTATION,
            lib_u8g2_byte_i2c, lib_u8g2_custom_cb);

        // Initialize display descriptor
        u8g2_InitDisplay(&g_u8g2);

        // Wake up display
        u8g2_SetPowerSave(&g_u8g2, 0);
    }

    // Initialize development kit button GPIO
    // -- Open button1 GPIO as input
    if (result != -1)
    {
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
    g_screen_id++;
    if (g_screen_id > SCR_GRAPHICS)
    {
        g_screen_id = SCR_LOGO;
    }
}

static void
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

static void
display_screen(screen_id_t scr_id)
{
    u8g2_ClearBuffer(&g_u8g2);

    switch (scr_id)
    {
    case SCR_LOGO:
        u8g2_DrawXBM(&g_u8g2, 0, 0, e14_logo_width, e14_logo_height, e14_logo_bits);
        break;

    case SCR_FONT:
        u8g2_SetFont(&g_u8g2, u8g2_font_oldwizard_tr);
        lib_u8g2_DrawCenteredStr(&g_u8g2, 10, "element14");

        u8g2_SetFont(&g_u8g2, u8g2_font_t0_22b_tr);
        lib_u8g2_DrawCenteredStr(&g_u8g2, 30, "element14");

        u8g2_SetFont(&g_u8g2, u8g2_font_helvB18_tr);
        lib_u8g2_DrawCenteredStr(&g_u8g2, 60, "element14");
        break;

    case SCR_GRAPHICS:
        u8g2_DrawBox(&g_u8g2, 0, 0, 30, 20);
        u8g2_DrawFrame(&g_u8g2, 98, 0, 30, 20);
        u8g2_DrawDisc(&g_u8g2, 64, 32, 20, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_LEFT);
        u8g2_DrawCircle(&g_u8g2, 64, 32, 30, U8G2_DRAW_ALL);
        u8g2_DrawFrame(&g_u8g2, 0, 44, 30, 20);
        u8g2_DrawBox(&g_u8g2, 98, 44, 30, 20);

        u8g2_SetFont(&g_u8g2, u8g2_font_unifont_t_symbols);
        u8g2_DrawGlyph(&g_u8g2, 106, 18, 0x2603);	/* dec 9731/hex 2603 Snowman */
        break;

    default:
        break;
    }

    u8g2_SendBuffer(&g_u8g2);
    return;
}


/* [] END OF FILE */
