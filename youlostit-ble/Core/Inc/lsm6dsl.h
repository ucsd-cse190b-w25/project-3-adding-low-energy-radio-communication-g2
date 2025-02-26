/*
 * lsm6dsl.h
 *
 *  Created on: Feb 4, 2025
 *      Author: cheng
 */

#ifndef LSM6DSL_H_
#define LSM6DSL_H_

#include <stm32l475xx.h>
#include <i2c.h>

#define IMU_ADDR 0b1101010
#define CTRL1_XL 0x10
#define INT1_CTRL 0x0D

#define OUTX_L_XL 0x28
#define OUTX_H_XL 0x29
#define OUTY_L_XL 0x2A
#define OUTY_H_XL 0x2B
#define OUTZ_L_XL 0x2C
#define OUTZ_H_XL 0x2D

void lsm6dsl_init();
void lsm6dsl_read_xyz(int16_t* x, int16_t* y, int16_t* z);

#endif /* LSM6DSL_H_ */
