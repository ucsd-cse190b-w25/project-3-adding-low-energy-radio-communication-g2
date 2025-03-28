/*
 * leds.c
 *
 *  Created on: Oct 3, 2023
 *      Author: schulman
 */


/* Include memory map of our MCU */
#include <stm32l475xx.h>

void leds_init()
{
  RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
  /* Configure PA5 and PB14 as an output by clearing all bits and setting the mode */
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;
  GPIOB->MODER &= ~GPIO_MODER_MODE14;
  GPIOB->MODER |= GPIO_MODER_MODE14_0;

  /* Configure the GPIO output as push pull (transistor for high and low) */
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT5;
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT14;

  /* Disable the internal pull-up and pull-down resistors */
  GPIOA->PUPDR &= GPIO_PUPDR_PUPD5;
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD14;

  /* Configure the GPIO to use low speed mode */
  GPIOA->OSPEEDR |= (0x3 << GPIO_OSPEEDR_OSPEED5_Pos);
  GPIOB->OSPEEDR |= (0x3 << GPIO_OSPEEDR_OSPEED14_Pos);

  /* Turn off the LED */
  GPIOA->ODR &= ~GPIO_ODR_OD5;
  GPIOB->ODR &= ~GPIO_ODR_OD14;
}

void leds_set(uint8_t led)
{
    switch (led) {
        case 0:
            GPIOA->ODR &= ~GPIO_ODR_OD5;
            GPIOB->ODR &= ~GPIO_ODR_OD14;
            break;
        case 1:
            //LED 2 is turned on
            GPIOA->ODR |= GPIO_ODR_OD5;
            GPIOB->ODR &= ~GPIO_ODR_OD14;
            break;
        case 2:
        	//LED 1 is turned on
            GPIOA->ODR &= ~GPIO_ODR_OD5;
            GPIOB->ODR |= GPIO_ODR_OD14;
            break;
        case 3:
        	//Both LEDs are on
            GPIOA->ODR |= GPIO_ODR_OD5;
            GPIOB->ODR |= GPIO_ODR_OD14;
            break;

    }
}


