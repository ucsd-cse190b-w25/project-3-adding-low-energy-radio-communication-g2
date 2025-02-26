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
#include "i2c.h"
#include "lsm6dsl.h"

int dataAvailable = 0;

SPI_HandleTypeDef hspi3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);

//Old code
#define ARR_LENGTH 16
#define MINUTE_COUNT 1200  // 1 minute worth of 50ms intervals

// Preamble: 10 01 10 01
// Rahul's Student ID (0596): 00 00 00 10 01 01 01 00
uint8_t arr[] = {0b10, 0b01, 0b10, 0b01, 0b00, 0b00, 0b00, 0b10, 0b01, 0b01, 0b01, 0b00, 0b00, 0b00, 0b00, 0b00}; // Wrote the sequence into an array
volatile int counter = 0;
volatile int arr_counter = 0;  // Counter for which part of the array we are displaying
volatile uint8_t minute_counter = 0; // counter for how many minutes have gone by

void TIM2_IRQHandler(void)
{

    if (TIM2->SR & TIM_SR_UIF) {  // check if interrupt status reg != 0
        TIM2->SR &= ~TIM_SR_UIF;  // turn it off

        arr_counter = (arr_counter + 1) % ARR_LENGTH;  // increment counter

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
  timer_init(TIM2);
  timer_set_ms(TIM2, 50);
  printf("Print works");
  i2c_init();
  lsm6dsl_init();

  int16_t x, y, z;
  int16_t last_x = 0, last_y = 0, last_z = 0;
  int stable_counter = 0;  // Count how many iterations values remain within threshold
  const int STABLE_THRESHOLD = 160;

  int _write(int file, char *ptr, int len) {
      int i = 0;
      for (i = 0; i < len; i++) {
          ITM_SendChar(*ptr++);
      }
      return len;
  }
  //Old code end
  HAL_Delay(10);

  uint8_t nonDiscoverable = 0;

  while (1)
  {
	  //Old code
	  lsm6dsl_read_xyz(&x, &y, &z);

	          // Convert values to match the scale
	          int16_t x_scaled = x / 16;
	          int16_t y_scaled = y / 16;
	          int16_t z_scaled = z / 16;

	          // Check if the change is within the stable threshold
	          if (abs(x_scaled - last_x) <= STABLE_THRESHOLD && abs(y_scaled - last_y) <= STABLE_THRESHOLD && abs(z_scaled - last_z) <= STABLE_THRESHOLD)
	          {
	              stable_counter++;
	              if (counter >= MINUTE_COUNT)
	              {
	              	if(minute_counter==0){
	                  	arr_counter = 0;
	                  }
	                  minute_counter+=1;
	                  uint8_t segment1 = (minute_counter >> 6) & 0x3; // Bits 7-8
	                  uint8_t segment2 = (minute_counter >> 4) & 0x3; // Bits 5-6
	                  uint8_t segment3 = (minute_counter >> 2) & 0x3; // Bits 3-4
	                  uint8_t segment4 = minute_counter & 0x3;
	                  arr[12] = segment1;
	                  arr[13] = segment2;
	                  arr[14] = segment3;
	                  arr[15] = segment4;
	                  counter = 0; //reset counter so it doesn't keep printing


	                  printf("Device has not moved for %d minute(s)\n", minute_counter);

	              }
	          }
	          else
	          {
	          	leds_set(0);
	          	arr_counter = 0;
	          	minute_counter = 0;
	              counter = 0; // Reset the counter when the thing moves
	          }

	          // Updating the compares
	          last_x = x_scaled;
	          last_y = y_scaled;
	          last_z = z_scaled;

	          if(minute_counter>0){
	          	leds_set(arr[arr_counter]);  // set LEDs based off current pattern
	          }
	  //Old code end
	  if(!nonDiscoverable && HAL_GPIO_ReadPin(BLE_INT_GPIO_Port,BLE_INT_Pin)){
	    catchBLE();
	  }else{
		  HAL_Delay(1000);
		  // Send a string to the NORDIC UART service, remember to not include the newline
		  unsigned char test_str[] = "youlostit BLE test";
		  updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, sizeof(test_str)-1, test_str);
	  }
	  // Wait for interrupt, only uncomment if low power is needed
	  //__WFI();
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
