/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_polo16_Pin GPIO_PIN_13
#define LED_polo16_GPIO_Port GPIOC
#define LED_polo36_Pin GPIO_PIN_14
#define LED_polo36_GPIO_Port GPIOC
#define LED_polo37_Pin GPIO_PIN_15
#define LED_polo37_GPIO_Port GPIOC
#define LED_polo6_Pin GPIO_PIN_0
#define LED_polo6_GPIO_Port GPIOA
#define LED_polo7_Pin GPIO_PIN_1
#define LED_polo7_GPIO_Port GPIOA
#define LED_polo8_Pin GPIO_PIN_2
#define LED_polo8_GPIO_Port GPIOA
#define LED_polo9_Pin GPIO_PIN_3
#define LED_polo9_GPIO_Port GPIOA
#define LED_polo10_Pin GPIO_PIN_4
#define LED_polo10_GPIO_Port GPIOA
#define LED_polo11_Pin GPIO_PIN_5
#define LED_polo11_GPIO_Port GPIOA
#define LED_polo12_Pin GPIO_PIN_6
#define LED_polo12_GPIO_Port GPIOA
#define LED_polo13_Pin GPIO_PIN_7
#define LED_polo13_GPIO_Port GPIOA
#define LED_polo14_Pin GPIO_PIN_0
#define LED_polo14_GPIO_Port GPIOB
#define LED_polo15_Pin GPIO_PIN_1
#define LED_polo15_GPIO_Port GPIOB
#define LED_polo16B2_Pin GPIO_PIN_2
#define LED_polo16B2_GPIO_Port GPIOB
#define LED_polo17_Pin GPIO_PIN_10
#define LED_polo17_GPIO_Port GPIOB
#define LED_polo18_Pin GPIO_PIN_11
#define LED_polo18_GPIO_Port GPIOB
#define LED_polo19_Pin GPIO_PIN_12
#define LED_polo19_GPIO_Port GPIOB
#define LED_polo20_Pin GPIO_PIN_13
#define LED_polo20_GPIO_Port GPIOB
#define LED_polo21_Pin GPIO_PIN_14
#define LED_polo21_GPIO_Port GPIOB
#define LED_polo22_Pin GPIO_PIN_15
#define LED_polo22_GPIO_Port GPIOB
#define LED_polo23_Pin GPIO_PIN_8
#define LED_polo23_GPIO_Port GPIOA
#define LED_polo24_Pin GPIO_PIN_9
#define LED_polo24_GPIO_Port GPIOA
#define LED_polo25_Pin GPIO_PIN_10
#define LED_polo25_GPIO_Port GPIOA
#define LED_polo26_Pin GPIO_PIN_11
#define LED_polo26_GPIO_Port GPIOA
#define LED_polo27_Pin GPIO_PIN_12
#define LED_polo27_GPIO_Port GPIOA
#define LED_polo28_Pin GPIO_PIN_15
#define LED_polo28_GPIO_Port GPIOA
#define LED_polo29_Pin GPIO_PIN_3
#define LED_polo29_GPIO_Port GPIOB
#define LED_polo30_Pin GPIO_PIN_4
#define LED_polo30_GPIO_Port GPIOB
#define LED_polo31_Pin GPIO_PIN_5
#define LED_polo31_GPIO_Port GPIOB
#define LED_polo32_Pin GPIO_PIN_6
#define LED_polo32_GPIO_Port GPIOB
#define LED_polo33_Pin GPIO_PIN_7
#define LED_polo33_GPIO_Port GPIOB
#define LED_polo34_Pin GPIO_PIN_8
#define LED_polo34_GPIO_Port GPIOB
#define LED_polo35_Pin GPIO_PIN_9
#define LED_polo35_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
