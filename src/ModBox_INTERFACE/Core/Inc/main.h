/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define input_type_Pin GPIO_PIN_9
#define input_type_GPIO_Port GPIOC
#define attack_Pin GPIO_PIN_0
#define attack_GPIO_Port GPIOA
#define release_Pin GPIO_PIN_1
#define release_GPIO_Port GPIOA
#define time_Pin GPIO_PIN_5
#define time_GPIO_Port GPIOA
#define feedback_Pin GPIO_PIN_6
#define feedback_GPIO_Port GPIOA
#define resolution_Pin GPIO_PIN_7
#define resolution_GPIO_Port GPIOA
#define preset_1_Pin GPIO_PIN_9
#define preset_1_GPIO_Port GPIOA
#define preset_2_Pin GPIO_PIN_10
#define preset_2_GPIO_Port GPIOA
#define preset_3_Pin GPIO_PIN_11
#define preset_3_GPIO_Port GPIOA
#define preset_4_Pin GPIO_PIN_12
#define preset_4_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
