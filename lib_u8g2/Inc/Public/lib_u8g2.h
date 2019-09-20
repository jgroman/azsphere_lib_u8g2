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

#include <stdint.h>

#include "../../../u8g2/csrc/u8x8.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set global I2C interface file descriptor for library use.
 *
 * This function must be called first before u8g2 functions can be used.
 *
 * @param fd_i2c I2C interface file descriptor.
 */
void 
lib_u8g2_set_i2c_fd(int fd_i2c);

/**
 * @brief Azure Sphere hardware custom I2C interface for u8x8 library.
 */
uint8_t
u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief Azure Sphere hardware custom delay and GPIO callback.
 */
uint8_t
lib_u8g2_custom_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
}
#endif

#endif // LIB_U8G2_H

/* [] END OF FILE */
