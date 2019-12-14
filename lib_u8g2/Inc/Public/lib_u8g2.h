/***************************************************************************//**
* @file    lib_u8g2.h
* @version 1.0.0
*
* @brief Custom Azure Sphere wrapper for u8g2 library.
*
* @author Jaroslav Groman
*
*******************************************************************************/

#ifndef LIB_U8G2_H
#define LIB_U8G2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../../../u8g2/csrc/u8x8.h"
// Comment line below if not using u8g2 functions
#include "../../../u8g2/csrc/u8g2.h"

#define I2C_STRUCTS_VERSION 1
#include <applibs/i2c.h>

/**
 * @brief Set OLED display I2C interface file descriptor and address
 *
 * This function must be called first before hardware dependent custom library
 * functions can be used.
 *
 * @param fd_i2c I2C interface file descriptor.
 * @param addr_i2c Device I2C address.
 */
void
lib_u8g2_set_i2c(int fd_i2c, I2C_DeviceAddress addr_i2c);

/**
 * @brief Azure Sphere hardware custom I2C interface for u8x8 library.
 */
uint8_t
lib_u8g2_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief Azure Sphere hardware custom delay and GPIO callback.
 */
uint8_t
lib_u8g2_custom_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief Draw centered string.
 */
u8g2_uint_t
lib_u8g2_DrawCenteredStr(u8g2_t *u8g2, u8g2_uint_t y, const char *s);

#ifdef __cplusplus
}
#endif

#endif // LIB_U8G2_H

/* [] END OF FILE */
