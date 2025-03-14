/*
 * lptim.c
 *
 *  Created on: Mar 12, 2025
 *      Author: cheng
 */

#include "lptim.h"
#include "leds.h"
#include <stm32l475xx.h>


void lptimer_init(LPTIM_TypeDef* timer)
{
//	timer->CR1 &= ~TIM_CR1_CEN;
//	timer->CNT = 0;//making the timer counter 0
//	timer->SR = 0;//making the interrupt flag 0
//	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; //adding the clock for TIM2
//	timer->PSC = 8000-1; // Making the clock frequency 1ms
//	timer->ARR = 49; // Setting the timer interrupt to 50ms
//	timer->DIER|=TIM_DIER_UIE;
//	NVIC_EnableIRQ(TIM2_IRQn);
//	NVIC_SetPriority(TIM2_IRQn, 0);
//	timer->CR1|= TIM_CR1_CEN; //Starting the timer

	timer->CR &= ~LPTIM_CR_ENABLE;

	// Enable the LSI clock
	RCC->CSR |= RCC_CSR_LSION;
	while ((RCC->CSR & RCC_CSR_LSIRDY) == 0);

	RCC->CCIPR &= ~RCC_CCIPR_LPTIM1SEL;  // Clear clock selection bits
	RCC->CCIPR |= (0b01 << RCC_CCIPR_LPTIM1SEL_Pos);  // Set 01 for LSI

	timer->CNT = 0;
	RCC->APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN;
	timer->CFGR |= (LPTIM_CFGR_PRESC_0 | LPTIM_CFGR_PRESC_2); // divide by 32 PRESC (1khz freq, 1ms period)
	timer->ARR = 49;
	timer->IER = LPTIM_IER_ARRMIE;  // Enable interrupt on match

	NVIC_EnableIRQ(LPTIM1_IRQn);
	NVIC_SetPriority(LPTIM1_IRQn, 0);

	timer->CR |= LPTIM_CR_ENABLE;
	timer->CR |= LPTIM_CR_CNTSTRT;
}

void lptimer_reset(LPTIM_TypeDef* timer)
{
	timer->CNT = 0; // Reseting the timer to zero
}

void lptimer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms)
{
	timer->ARR = period_ms-1; // Setting the timer interrupt to whatever is passed in
}
