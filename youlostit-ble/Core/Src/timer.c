/*
 * timer.c
 *
 *  Created on: Oct 5, 2023
 *      Author: schulman
 */

#include "timer.h"
#include <stm32l475xx.h>


void timer_init(TIM_TypeDef* timer)
{
	timer->CR1 &= ~TIM_CR1_CEN;
	timer->CNT = 0;//making the timer counter 0
	timer->SR = 0;//making the interrupt flag 0
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; //adding the clock for TIM2
	timer->PSC = 4000-1; // Making the clock frequency 1ms
	timer->ARR = 49; // Setting the timer interrupt to 50ms
	timer->DIER|=TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 0);
	timer->CR1|= TIM_CR1_CEN; //Starting the timer
}

void timer_reset(TIM_TypeDef* timer)
{
	timer->CNT = 0; // Reseting the timer to zero
}

void timer_set_ms(TIM_TypeDef* timer, uint16_t period_ms)
{
	timer->ARR = period_ms-1; // Setting the timer interrupt to whatever is passed in
}
