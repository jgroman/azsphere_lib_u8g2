/***************************************************************************//**
* @file    lib_u8g2.h
* @version 1.0.0
*
* @brief Custom Azure Sphere wrapper for u8g2 library.
*
* @author Jaroslav Groman
*
* @date
*
*******************************************************************************/

#ifndef __LIBOLEDSSD1306_H__
#define __LIBOLEDSSD1306_H__

#include <stdint.h>

#include "../../../u8g2/csrc/u8x8.h"

#ifdef __cplusplus
extern "C" {
#endif

    void lib_u8g2_set_i2c_fd(int);

    uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

    uint8_t lib_u8g2_custom_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
}
#endif

#endif // __LIBOLEDSSD1306_H__

/* [] END OF FILE */
