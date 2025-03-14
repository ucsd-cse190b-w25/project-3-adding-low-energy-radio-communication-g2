/*
 * lptim.h
 *
 *  Created on: Mar 12, 2025
 *      Author: cheng
 */

#ifndef LPTIM_H_
#define LPTIM_H_

/* Include the type definitions for the timer peripheral */
#include <stm32l475xx.h>

void lptimer_init(LPTIM_TypeDef* timer);
void lptimer_reset(LPTIM_TypeDef* timer);
void lptimer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms);

#endif /* LPTIM_H_ */
