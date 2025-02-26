/*
 * lsm6dsl.c
 *
 *  Created on: Feb 4, 2025
 *      Author: cheng
 */

#include "lsm6dsl.h"

void lsm6dsl_init() {
//	i2c_init();  // ensure that i2c is initialized

	// CTRL1_XL = 0x60
	uint8_t ctrl1_xl_data[2] = {CTRL1_XL, 0x60};
	i2c_transaction(IMU_ADDR, 0, ctrl1_xl_data, 2);

	// INT1_CTRL = 0x01
	uint8_t int1_ctrl_data[2] = {INT1_CTRL, 0x01};
	i2c_transaction(IMU_ADDR, 0, int1_ctrl_data, 2);
}

void lsm6dsl_read_xyz(int16_t* x, int16_t* y, int16_t* z) {
	uint8_t data[7] = {OUTX_L_XL};

	i2c_transaction(IMU_ADDR, 1, data, 7);

	*x = data[2] << 8 | data[1];
	*y = data[4] << 8 | data[3];
	*z = data[6] << 8 | data[5];
}
