/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
//#include "ble_commands.h"
#include "ble.h"

#include <stdlib.h>
#include "leds.h"
#include "lptim.h"
#include "i2c.h"
#include "lsm6dsl.h"
#include "stm32l4xx_hal_pwr.h"

#define BLE_MAX_PACKET_SIZE 20  // Typical BLE packet size
#define TAG_NAME "PrivTag"

int dataAvailable = 0;

SPI_HandleTypeDef hspi3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);

//Old code
#define TIME_PERIOD 200
#define MINUTE_MS 60000 // # of ms in a minute, should be 60000 unless if scaling down for debugging
#define MINUTE_COUNT (MINUTE_MS / TIME_PERIOD)
#define SEC_COUNT (1000 / TIME_PERIOD)

// Preamble: 10 01 10 01
// Rahul's Student ID (0596): 00 00 00 10 01 01 01 00
volatile int counter = 0;
volatile uint8_t minute_counter = 0; // counter for how many minutes have gone by
int bool = 1;

void sendMissingAlert(int seconds) {
	char message[50];  // Buffer for the formatted string
	snprintf(message, sizeof(message), "PrivTag has been missing for %d seconds", seconds);

	int message_len = strlen(message);
	int offset = 0;

	while (offset < message_len) {
		int chunk_size = (message_len - offset > BLE_MAX_PACKET_SIZE) ? BLE_MAX_PACKET_SIZE : (message_len - offset);

		// Send each chunk as a standalone notification
		updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, chunk_size, (uint8_t*)&message[offset]);

		offset += chunk_size;
		HAL_Delay(50);  // Small delay to allow BLE module to process
	}
}

void LPTIM1_IRQHandler(void)
{
	if (LPTIM1->ISR & LPTIM_ISR_ARRM) {
        LPTIM1->ICR |= LPTIM_ICR_ARRMCF;  // Clear interrupt flag
        // Your custom wakeup or callback code here
        counter += 1;
    }
}
int _write(int file, char *ptr, int len) {
	int i = 0;
	for (i = 0; i < len; i++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}
//Old code end

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_SPI3_Init();

	//RESET BLE MODULE
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_SET);

	ble_init();
	//Old code
	leds_init();
	lptimer_init(LPTIM1);
	lptimer_set_ms(LPTIM1, TIME_PERIOD);
	HAL_Delay(500);
	i2c_init();
	lsm6dsl_init();

	RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART2EN;

	int16_t x, y, z;
	int16_t last_x = 0, last_y = 0, last_z = 0;
	int stable_counter = 0;  // Count how many iterations values remain within threshold
	const int STABLE_THRESHOLD = 160;

	//Old code end
	HAL_Delay(10);

	uint8_t nonDiscoverable = 0;
	disconnectBLE();
	while (1)
	{
		RCC->APB1ENR1 |= (RCC_APB1ENR1_I2C1EN); // I2C
		//Old code
		lsm6dsl_read_xyz(&x, &y, &z);
		RCC->APB1ENR1 &= ~(RCC_APB1ENR1_I2C1EN); // I2C

		// Convert values to match the scale
		int16_t x_scaled = x / 16;
		int16_t y_scaled = y / 16;
		int16_t z_scaled = z / 16;

		// Check if the change is within the stable threshold
		if (abs(x_scaled - last_x) <= STABLE_THRESHOLD && abs(y_scaled - last_y) <= STABLE_THRESHOLD && abs(z_scaled - last_z) <= STABLE_THRESHOLD)
		{
			stable_counter++;
			if (counter >= MINUTE_COUNT && bool && counter%(SEC_COUNT * 10)==0)
			{
				setDiscoverability(1);
				leds_set(2);
				//updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, sizeof(test_str)-1, test_str);

				bool = 0;
				int missing_seconds = counter / SEC_COUNT;
				sendMissingAlert(missing_seconds);

			}
			else if(counter >= MINUTE_COUNT && !bool && counter%(SEC_COUNT * 10)!=0)
			{
				bool = 1;
			}
		}
		else
		{
			disconnectBLE();
			leds_set(0);
			minute_counter = 0;
			counter = 0; // Reset the counter when the thing moves
		}

		// Updating the compares
		last_x = x_scaled;
		last_y = y_scaled;
		last_z = z_scaled;

		//Old code end

		if(!nonDiscoverable && HAL_GPIO_ReadPin(BLE_INT_GPIO_Port,BLE_INT_Pin)){
			catchBLE();
		}

		/*
		 * Turn off interrupts
		 *
		 */

		RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;        // SPI
		RCC->AHB2ENR &= ~(RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
		HAL_SuspendTick();  // Stop SysTick timer to save power
//		__WFI();
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

		HAL_ResumeTick();   // Resume SysTick when waking up
		RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
		RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;        // SPI
	}
}

/**
 * @brief System Clock Configuration
 * @attention This changes the System clock frequency, make sure you reflect that change in your timer
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	// This lines changes system clock frequency
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief SPI3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI3_Init(void)
{

	/* USER CODE BEGIN SPI3_Init 0 */

	/* USER CODE END SPI3_Init 0 */

	/* USER CODE BEGIN SPI3_Init 1 */

	/* USER CODE END SPI3_Init 1 */
	/* SPI3 parameter configuration*/
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi3.Init.CRCPolynomial = 7;
	hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&hspi3) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI3_Init 2 */

	/* USER CODE END SPI3_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIO_LED1_GPIO_Port, GPIO_LED1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(BLE_CS_GPIO_Port, BLE_CS_Pin, GPIO_PIN_SET);


	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin : BLE_INT_Pin */
	GPIO_InitStruct.Pin = BLE_INT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BLE_INT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : GPIO_LED1_Pin BLE_RESET_Pin */
	GPIO_InitStruct.Pin = GPIO_LED1_Pin|BLE_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : BLE_CS_Pin */
	GPIO_InitStruct.Pin = BLE_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(BLE_CS_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
