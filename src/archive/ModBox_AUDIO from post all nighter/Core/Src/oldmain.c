///*
// * oldmain.c
// *
// *  Created on: Apr 23, 2026
// *      Author: bhavyasurapaneni
// */
///* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * @file           : main.c
//  * @brief          : Main program body
//  ******************************************************************************
//  * @attention
//  *
//  * Copyright (c) 2026 STMicroelectronics.
//  * All rights reserved.
//  *
//  * This software is licensed under terms that can be found in the LICENSE file
//  * in the root directory of this software component.
//  * If no LICENSE file comes with this software, it is provided AS-IS.
//  *
//  ******************************************************************************
//  */
///* USER CODE END Header */
///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//#include "adc.h"
//#include "dma.h"
//#include "tim.h"
//#include "usart.h"
//#include "gpio.h"
//
///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
//#include <string.h>
//#include <stdio.h>
//#include <stdbool.h>
//#include "audio_pipeline.h"
///* USER CODE END Includes */
//
///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
//typedef struct {
//    uint8_t input_mode;
//    uint8_t preset_id;
//    uint8_t attack;
//    uint8_t release;
//    uint8_t time;
//    uint8_t feedback;
//    uint8_t resolution;
//} SynthParams_t;
//
//void Debug_Log(const char* msg) {
//    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
//    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
//}
///* USER CODE END PTD */
//
///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */
//
///* USER CODE END PD */
//
///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */
//
///* USER CODE END PM */
//
///* Private variables ---------------------------------------------------------*/
//
///* USER CODE BEGIN PV */
//SynthParams_t synth = {0};
//uint8_t rx_buffer[7];
//volatile bool new_data_flag = false;
//
//volatile uint32_t raw_val = 0;
//volatile float signal_in = 0.0f;
//AudioPipeline_t myPipeline;
//
//volatile bool audio_ready = false; // Flag triggered by Timer 2
//volatile uint32_t adc_val = 0;
//volatile uint8_t adc_ready_flag = 0;
//char msg[64];
///* USER CODE END PV */
//
///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
///* USER CODE BEGIN PFP */
//
///* USER CODE END PFP */
//
///* Private user code ---------------------------------------------------------*/
///* USER CODE BEGIN 0 */
//
///* USER CODE END 0 */
//
///**
//  * @brief  The application entry point.
//  * @retval int
//  */
//
////int main(void) {
////    // 1. Reset of all peripherals, Initializes the Flash interface and the Systick.
////    HAL_Init();
////
////    // 2. Configure the system clock (Ensure this matches your 84MHz or 100MHz setup)
////    SystemClock_Config();
////
////    // 3. Initialize all configured peripherals
////    MX_GPIO_Init();
////    MX_TIM2_Init();
////
////    /* * 4. START THE PWM
////     * This connects the internal timer logic to the PA5 physical pin.
////     */
////    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) {
////        Error_Handler();
////    }
////
////    /* * 5. SET THE DUTY CYCLE
////     * Your Period is 1049.
////     * Setting Pulse to 525 creates a 50% duty cycle square wave.
////     */
////    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 525);
////
////    while (1) {
////        // Just keep the CPU alive.
////        // If you have an onboard LED (like PC13 or PA5), you can toggle it here
////        // to prove the code hasn't crashed.
////        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
////        HAL_Delay(500);
////    }
////}
//int main(void) {
//    HAL_Init();
//    // 1. FREE PA15 - Crucial if USART1 is even initialized in the background
//
//    SystemClock_Config();
//
//    /* Initialize Peripherals */
//    MX_GPIO_Init();
//    MX_DMA_Init();
//    MX_USART2_UART_Init(); // Printing to PC
//    MX_USART1_UART_Init();
//    MX_ADC1_Init();        // Reading VCO/Pot
//    MX_TIM2_Init();        // PWM on PA5
//
//    /* Start Peripherals */
//    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
//    HAL_ADC_Start(&hadc1);
//
//    char msg[64];
//    uint32_t last_print_tick = 0;
//
//    while (1) {
//            // A. Read local ADC (for signal processing later)
//            if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
//                uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
//                HAL_ADC_Start(&hadc1); // Restart for next loop
//
//                // B. If new data arrived from USART1, let's use it!
//                if (new_data_flag) {
//                    new_data_flag = false;
//
//                    // For this test: map the 'attack' byte (rx_buffer[2]) to the PWM
//                    uint32_t remote_val = rx_buffer[2];
//                    uint32_t pulse = (remote_val * 1049) / 255;
//                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
//
//                    // Toggle LED to show packet received
//                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
//                }
//
//                // C. Periodic Debug Print
//                if (HAL_GetTick() - last_print_tick > 500) {
//                    int len = snprintf(msg, sizeof(msg), "Remote Attack: %d\r\n", rx_buffer[2]);
//                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 10);
//                    last_print_tick = HAL_GetTick();
//                }
//            }
//        }
//}
////int main(void)
////{
////
////  /* USER CODE BEGIN 1 */
////
////  /* USER CODE END 1 */
////
////  /* MCU Configuration--------------------------------------------------------*
////  /* USER CODE BEGIN Init */
////
////	HAL_Init();
////  /* USER CODE END Init */
////
////  /* Configure the system clock */
////  SystemClock_Config();
////
////  /* USER CODE BEGIN SysInit */
////  //  AudioSystem_Init();
////  /* USER CODE END SysInit */
////
////  /* Initialize all configured peripherals */
////  MX_GPIO_Init();
////  MX_DMA_Init();
////  MX_USART2_UART_Init();
////  MX_ADC1_Init();
////  MX_TIM2_Init();
////  // MX_USART1_UART_Init();
////  /* USER CODE BEGIN 2 */
////  HAL_UART_Transmit(&huart2, (uint8_t*)"STARTING SYSTEM\r\n", 17, 100);
////  Pipeline_Init(&myPipeline);
////  HAL_TIM_Base_Start_IT(&htim2);
////  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
////  // HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
////  HAL_ADC_Start(&hadc1);
////
////
////  /* USER CODE END 2 */
////
////  /* Infinite loop */
////  /* USER CODE BEGIN WHILE */
////  while (1)
////  {
////    /* USER CODE END WHILE */
////
////    /* USER CODE BEGIN 3 */
////
////	  HAL_ADC_Start_IT(&hadc1);
////
////	  // -------------------------------------------------------
////	  // 1. NON-BLOCKING PARAMETER SYNC (Runs every 20ms)
////	  // -------------------------------------------------------
////	  static uint32_t last_sync = 0;
////
////// TESTING THE ENVELOPE PRESET
////
//////	  static uint32_t last_gate_flip = 0;
//////	  static bool virtual_gate = false;
//////
//////
//////	  if (HAL_GetTick() - last_gate_flip > 1000) { // Every 1 second
//////	      virtual_gate = !virtual_gate;
//////	      last_gate_flip = HAL_GetTick();
//////
//////	      if (virtual_gate) {
//////	          // "Press Key" -> Moves state to ATTACK
//////	          Envelope_Trigger(&myPipeline.envelope);
//////	      } else {
//////	          // "Release Key" -> Moves state to RELEASE
//////	          Envelope_Release(&myPipeline.envelope);
//////	      }
//////
//////	  }
////
////	  if (HAL_GetTick() - last_sync > 20) {
//////		  myPipeline.active_preset = synth.preset_id;
////		  myPipeline.source    			= (synth.input_mode == 0) ? SOURCE_CV : SOURCE_MIC;
////
////		  // Map Envelope (Higher UART value = Faster Attack/Release)
////		  myPipeline.envelope.attack_rate  = (float)synth.attack / 10000.0f;
////		  myPipeline.envelope.release_rate = (float)synth.release / 10000.0f;
////
////		  // Map Echo (Feedback constrained 0.0 to 0.94 to avoid runaway audio)
////
////		  myPipeline.echo.feedback      = (float)synth.feedback / 270.0f;
////		  myPipeline.echo.delay_samples = (uint32_t)(synth.time * 15);
////
////		  // Map Bit Crusher
////		  myPipeline.discretizer.resolution = (synth.resolution / 32) + 1; // 1 to 8 bits
////
////		  last_sync = HAL_GetTick();
////	  }
////
////	  // -------------------------------------------------------
////	  // 2. HIGH-SPEED AUDIO ENGINE (Runs at 8kHz)
////	  // -------------------------------------------------------
////	  if (audio_ready) {
////	      audio_ready = false;
////
////		  float voltage = (adc_val * 3.3f) / 4095.0f;
////
////		  // A. Read Gate Pin (NEED TO CHANGE THIS PIN)
////
////		  uint32_t raw_val = HAL_ADC_GetValue(&hadc1);
////		  float signal_in = ((float)raw_val - 745.0f) / 745.0f; // changing because the input was around 0-1.2V
////		  uint32_t current_raw = adc_val;
////
////
////		  // 2. SLOW PRINTING (Only every 100ms) print for debugging
////		  //but printing will mess the the pwm so comment out when running
////		  static uint32_t last_print = 0;
////		  if (HAL_GetTick() - last_print > 100) {
////			  char debug_msg[64];
////			  int len = sprintf(debug_msg, "ADC: %lu | Sig: %.2f\r\n", current_raw, signal_in);
////			  HAL_UART_Transmit(&huart2, (uint8_t*)debug_msg, len, 10);
////			  last_print = HAL_GetTick();
////		        }
////
////
////		  // C. Process Signal through your Effects Pipeline
////		  float effect_out = Pipeline_Process(&myPipeline, signal_in, true);
////
////
////		  // D. Apply VCA (Envelope Volume)
////		  // (Envelope_Update is already called inside Pipeline_Process based on your audio_pipeline code)
////		  float final_out = effect_out; // If Pipeline_Process outputs the fully gated signal
////
////	      // E. PWM Output Translation
////		  // Center the float at 525 (half of 1050)
////		  uint32_t pwm_val = (uint32_t)((final_out + 1.0f) * 524.5f);
////
////		  // Constrain to prevent speaker popping/timer overflow
////		  if (pwm_val > 1049) pwm_val = 1049;
////
////		  // Send to Timer Pin!
////		  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_val);
////	  }
////	}
////  /* USER CODE END 3 */
////}
//
///**
//  * @brief System Clock Configuration
//  * @retval None
//  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//
//  /** Configure the main internal regulator output voltage
//  */
//  __HAL_RCC_PWR_CLK_ENABLE();
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
//
//  /** Initializes the RCC Oscillators according to the specified parameters
//  * in the RCC_OscInitTypeDef structure.
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
//  RCC_OscInitStruct.PLL.PLLM = 16;
//  RCC_OscInitStruct.PLL.PLLN = 336;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
//  RCC_OscInitStruct.PLL.PLLQ = 4;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}
//
///* USER CODE BEGIN 4 */
//
//// This function fires automatically exactly 8,000 times a second
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//    if (htim->Instance == TIM2) {
//        audio_ready = true; // Tell the main loop to process the next sample!
//    }
//}
//
//
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//	if (huart->Instance == USART1) { // Change to your specific UART instance
//
//		// 1. Populate the raw synth struct
//		synth.input_mode = rx_buffer[0];
//		synth.preset_id  = rx_buffer[1];
//		synth.attack     = rx_buffer[2];
//		synth.release    = rx_buffer[3];
//		synth.time       = rx_buffer[4];
//		synth.feedback   = rx_buffer[5];
//		synth.resolution = rx_buffer[6];
//
//		// 2. Map the raw integers (0-255) to the DSP Pipeline floats (0.0f - 1.0f)
//		myPipeline.source          = synth.input_mode;
//		myPipeline.active_preset   = synth.preset_id;
//
//		// Scale the 8-bit knob data down to decimals for the audio math
//		// You might need to tweak the division here if attack/release need specific curves
//		myPipeline.envelope.attack_rate  = (float)synth.attack / 255.0f;
//		myPipeline.envelope.release_rate = (float)synth.release / 255.0f;
//		myPipeline.echo.time             = (float)synth.time / 255.0f;
//		myPipeline.echo.feedback         = (float)synth.feedback / 255.0f;
//
//		// (Assuming you add resolution to your pipeline later for the bitcrusher)
//		// myPipeline.bitcrush_res       = synth.resolution;
//
//		// 3. Print debug info
//		char msg[100];
//		// Note: Added \r\n for clean serial monitor formatting
//		sprintf(msg, "Mode: %d | Pre: %d | A:%d REL:%d T:%d F:%d RES: %d\r\n",
//				synth.input_mode, synth.preset_id, synth.attack,
//				synth.release, synth.time, synth.feedback, synth.resolution);
//		Debug_Log(msg);
//
//		// 4. Re-arm the interrupt to listen for the next 7-byte packet!
//		HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
//	}
//}
//
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
//{
//    if(hadc->Instance == ADC1)
//    {
//        adc_val = HAL_ADC_GetValue(hadc);
//        adc_ready_flag = 1; // Signal the main loop that data is ready
//    }
//}
//
///* USER CODE END 4 */
//
///**
//  * @brief  This function is executed in case of error occurrence.
//  * @retval None
//  */
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* User can add his own implementation to report the HAL error return state */
//  __disable_irq();
//  while (1)
//  {
//  }
//  /* USER CODE END Error_Handler_Debug */
//}
//#ifdef USE_FULL_ASSERT
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t *file, uint32_t line)
//{
//  /* USER CODE BEGIN 6 */
//  printf("Wrong parameters value: file %s on line %d\r\n", file, line)
//  /* USER CODE END 6 */
//}
//#endif /* USE_FULL_ASSERT */
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
/////*  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body */
////
////// include
////
////#include "main.h"
////#include "adc.h"
////#include "dma.h"
////#include "tim.h"
////#include "usart.h"
////#include "gpio.h"
////#include "mic.h"
////
/////* Private includes ----------------------------------------------------------*/
////#include <string.h>
////#include <stdio.h>
////#include <stdbool.h>
////#include "audio_pipeline.h"
////
/////* Private typedef -----------------------------------------------------------*/
////typedef struct {
////    uint8_t input_mode;
////    uint8_t preset_id;
////    uint8_t attack;
////    uint8_t release;
////    uint8_t time;
////    uint8_t feedback;
////    uint8_t resolution;
////} SynthParams_t;
////
////void Debug_Log(const char* msg) {
////    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
////    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
////}
////
/////* Private variables ---------------------------------------------------------*/
////
////SynthParams_t synth = {0};
////SynthParams_t last_synth = {0}; // Our "shadow copy"
////uint8_t rx_buffer[7];
////volatile uint32_t raw_val = 0;
////volatile float signal_in = 0.0f;
////AudioPipeline_t myPipeline;
////
////volatile bool audio_ready = false; // Flag triggered by Timer 2
////volatile uint32_t adc_val = 0;
////volatile uint8_t adc_ready_flag = 0;
////volatile bool blink_led = false;
////volatile bool new_uart_data = false;
////char msg[64];
////
/////* Private function prototypes -----------------------------------------------*/
////void SystemClock_Config(void);
////
////int main(void){
////  HAL_Init();
////
////  SystemClock_Config();
////  MX_GPIO_Init();
////  MX_DMA_Init();
////  MX_USART2_UART_Init();
////  MX_ADC1_Init();
////  MX_TIM2_Init();
////  MX_USART1_UART_Init();
//////  HAL_UART_Transmit(&huart2, (uint8_t*)"STARTING SYSTEM\r\n", 17, 100);
////  Pipeline_Init(&myPipeline);
////  HAL_TIM_Base_Start_IT(&htim2);
////  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
////  HAL_UART_Receive_DMA(&huart1, rx_buffer, 7);
////  HAL_ADC_Start(&hadc1);
////
////  while (1)
////  {
////	  static uint32_t last_check = 0;
////	  static uint32_t led_off_time = 0;
////	  static uint32_t last_adc_print = 0;
//////	  if (HAL_GetTick() - last_adc_print > 500) { // Every 500ms
//////	      last_adc_print = HAL_GetTick();
//////
//////	      // 1. Start the conversion
//////	      HAL_ADC_Start(&hadc1);
//////
//////	      // 2. Wait for it to finish (Timeout of 10ms is plenty)
//////	      if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
//////	          // 3. Get the value (0 to 4095)
//////	          uint32_t pa0_val = HAL_ADC_GetValue(&hadc1);
//////
//////	          // 4. Format and Print
//////	          char adc_msg[32];
//////	          snprintf(adc_msg, sizeof(adc_msg), "PA0: %lu", pa0_val);
//////	          Debug_Log(adc_msg);
//////	      }
//////
//////	      // 5. Stop the ADC to save power/resources
//////	      HAL_ADC_Stop(&hadc1);
//////	  }
////
////	  if (new_uart_data) {
////		  new_uart_data = false;
////
////		  synth.input_mode = rx_buffer[0];
////		  synth.preset_id  = rx_buffer[1];
////		  synth.attack     = rx_buffer[2];
////		  synth.release    = rx_buffer[3];
////		  synth.time       = rx_buffer[4];
////		  synth.feedback   = rx_buffer[5];
////		  synth.resolution = rx_buffer[6];
////
////		  myPipeline.source          = synth.input_mode;
////		  myPipeline.active_preset   = synth.preset_id;
////
////		  myPipeline.source = (synth.input_mode == 0) ? SOURCE_CV : SOURCE_MIC;
////
////		  // scaling knob data
////		  myPipeline.envelope.attack_rate  = (float)synth.attack / 255.0f;
////		  myPipeline.envelope.release_rate = (float)synth.release / 255.0f;
////		  myPipeline.echo.time             = (float)synth.time / 255.0f;
////		  myPipeline.echo.feedback         = (float)synth.feedback / 255.0f;
////
////		  // (Assuming you add resolution to your pipeline later for the bitcrusher)
////		  // myPipeline.bitcrush_res       = synth.resolution;
////
////		  // debug info
//////		  char msg[100];
//////		  sprintf(msg, "Mode: %d | Pre: %d | A:%d REL:%d T:%d F:%d RES: %d\r\n",
//////				synth.input_mode, synth.preset_id, synth.attack,
//////				synth.release, synth.time, synth.feedback, synth.resolution);
//////		  Debug_Log(msg);
////
////	  }
////
////	  if (blink_led) {
////	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
////	      led_off_time = HAL_GetTick() + 50;
////	      blink_led = false;
////	  }
////	  if (led_off_time > 0 && HAL_GetTick() >= led_off_time) {
////	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
////	      led_off_time = 0;
////	  }
////
////	  // -------------------------------------------------------
////	  // 2. HIGH-SPEED AUDIO ENGINE (Runs at 8kHz)
////	  // -------------------------------------------------------
////	  static uint32_t last_heartbeat = 0;
////	  if (HAL_GetTick() - last_heartbeat > 1000) {
////		  // This is safe because it only blocks the CPU once a second
//////		  HAL_UART_Transmit(&huart2, (uint8_t*)"ALIVE\r\n", 7, 10);
//////		  last_heartbeat = HAL_GetTick();
////	  }
////
////	  if (audio_ready) {
////	      audio_ready = false;
////	      float current_sample = 0.0f;
////
////	      if (myPipeline.source == SOURCE_MIC) {
////	          current_sample = Mic_ReadSample();
////	      }
////	      else {
////	          current_sample = ((float)adc_val - 2048.0f) / 2048.0f;
////	      }
////
////	      bool gate_state = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);
////	      float output = Pipeline_Process(&myPipeline, current_sample, gate_state);
////
////	      // Output to PWM
////	      uint32_t pwm_val = (uint32_t)((output + 1.0f) * 524.5f);
////	      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (pwm_val > 1049) ? 1049 : pwm_val);
////	  }
////  }
////}
////
/////**
////  * @brief System Clock Configuration
////  */
////void SystemClock_Config(void) {
////  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
////  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
////
////  /** Configure the main internal regulator output voltage
////  */
////  __HAL_RCC_PWR_CLK_ENABLE();
////  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
////
////  /** Initializes the RCC Oscillators according to the specified parameters
////  * in the RCC_OscInitTypeDef structure.
////  */
////  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
////  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
////  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
////  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
////  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
////  RCC_OscInitStruct.PLL.PLLM = 16;
////  RCC_OscInitStruct.PLL.PLLN = 336;
////  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
////  RCC_OscInitStruct.PLL.PLLQ = 4;
////  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
////    Error_Handler();
////  }
////
////  /** Initializes the CPU, AHB and APB buses clocks
////  */
////  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
////                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
////  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
////  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
////  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
////  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
////
////  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
////  {
////    Error_Handler();
////  }
////}
////
////
////// This function fires automatically exactly 8,000 times a second
////void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
////    if (htim->Instance == TIM2) {
////        audio_ready = true; // Tell the main loop to process the next sample!
////    }
////}
////
////
////void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
////    if (huart->Instance == USART1) {
////    	blink_led = true;
////        new_uart_data = true;
////        HAL_UART_Receive_DMA(&huart1, rx_buffer, 7);
////    }
////}
////
////
////void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
////{
////    if(hadc->Instance == ADC1)
////    {
////        adc_val = HAL_ADC_GetValue(hadc);
////        adc_ready_flag = 1; // Signal the main loop that data is ready
////    }
////}
////
////void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
////    if (huart->Instance == USART1) {
////        // Clear all error flags
////        __HAL_UART_CLEAR_OREFLAG(huart);
////        __HAL_UART_CLEAR_NEFLAG(huart);
////        __HAL_UART_CLEAR_FEFLAG(huart);
////
////        // Re-arm so we can try again
////        HAL_UART_Receive_DMA(&huart1, rx_buffer, 7);
////    }
////}
/////**
////  * @brief  This function is executed in case of error occurrence.
////  */
////void Error_Handler(void)
////{
////  /* User can add his own implementation to report the HAL error return state */
////  __disable_irq();
////  while (1)
////  {
////  }
////}
////#ifdef USE_FULL_ASSERT
/////**
////  * @brief  Reports the name of the source file and the source line number
////  *         where the assert_param error has occurred.
////  */
////void assert_failed(uint8_t *file, uint32_t line)
////{
////  printf("Wrong parameters value: file %s on line %d\r\n", file, line)
////}
////#endif /* USE_FULL_ASSERT */
//
//
