/***************************************************************************//**
* @file    lib_u8g2.c
* @version 1.0.0
*
* @brief Custom Azure Sphere wrapper for u8g2 library.
*
* @author Jaroslav Groman
*
*******************************************************************************/

#include <time.h>
#include <stdio.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/i2c.h>

#include <lib_u8g2.h>

// I2C file descriptor for I2C operations
static int g_i2c_fd = -1;

/**
 * @brief Set library global I2C file descriptor.
 */
void 
lib_u8g2_set_i2c_fd(int fd) {
    g_i2c_fd = fd;
}

/**
 * @brief Azure Sphere hardware custom I2C interface for u8x8 library
 */
uint8_t
u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    // u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER 
    // and END_TRANSFER
    static uint8_t buffer[32];
    static uint8_t buf_idx;
    uint8_t *data;

    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while (arg_int > 0)
            {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
        break;

        case U8X8_MSG_BYTE_INIT:
        break;

        case U8X8_MSG_BYTE_SET_DC:
        break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
        break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            I2CMaster_Write(g_i2c_fd, u8x8_GetI2CAddress(u8x8), buffer, buf_idx);
        break;

        default:
            return 0;
    }

    return 1;
}

/**
 * @brief Azure Sphere hardware custom delay and GPIO callback
 */
uint8_t 
lib_u8g2_custom_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    struct timespec sleep_time;

    switch (msg) 
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        break;

        case U8X8_MSG_DELAY_MILLI:
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 1000000;
            nanosleep(&sleep_time, NULL);
        break;

        case U8X8_MSG_DELAY_10MICRO:
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 10000;
            nanosleep(&sleep_time, NULL);
        break;

        case U8X8_MSG_DELAY_100NANO:
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 100;
            nanosleep(&sleep_time, NULL);
        break;

        /* If you want to use a software interface or have these pins then you
        *  need to read and write them
        case U8X8_MSG_GPIO_SPI_CLOCK:
        case U8X8_MSG_GPIO_SPI_DATA:
        case U8X8_MSG_GPIO_CS:
        case U8X8_MSG_GPIO_DC:
        case U8X8_MSG_GPIO_RESET:
        case U8X8_MSG_GPIO_SPI_CLOCK:
        case U8X8_MSG_GPIO_SPI_DATA:
        case U8X8_MSG_GPIO_D0:
        case U8X8_MSG_GPIO_D1:
        case U8X8_MSG_GPIO_D2:
        case U8X8_MSG_GPIO_D3:
        case U8X8_MSG_GPIO_D4:
        case U8X8_MSG_GPIO_D5:
        case U8X8_MSG_GPIO_D6:
        case U8X8_MSG_GPIO_D7:
        case U8X8_MSG_GPIO_E:
        case U8X8_MSG_GPIO_I2C_CLOCK:
        case U8X8_MSG_GPIO_I2C_DATA:
        break;  */

        default:
        break;
    }
    return 1;
}

/* [] END OF FILE */
