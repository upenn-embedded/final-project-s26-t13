/*  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body */

// include

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "mic.h"

/* Private includes ----------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "audio_pipeline.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    uint8_t input_mode;
    uint8_t preset_id;
    uint8_t attack;
    uint8_t release;
    uint8_t time;
    uint8_t feedback;
    uint8_t resolution;
} SynthParams_t;

void Debug_Log(const char* msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
}

/* Private variables ---------------------------------------------------------*/

SynthParams_t synth = {0};
SynthParams_t last_synth = {0}; // Our "shadow copy"
uint8_t rx_buffer[7];
volatile uint32_t raw_val = 0;
volatile float signal_in = 0.0f;
AudioPipeline_t myPipeline;

volatile bool audio_ready = false; // Flag triggered by Timer 2
volatile uint32_t adc_val = 0;
volatile uint8_t adc_ready_flag = 0;
volatile bool blink_led = false;
volatile bool new_uart_data = false;
char msg[64];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

int main(void){
  HAL_Init();
  SystemClock_Config();
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  HAL_UART_Transmit(&huart2, (uint8_t*)"STARTING SYSTEM\r\n", 17, 100);
  Pipeline_Init(&myPipeline);
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
  HAL_ADC_Start(&hadc1);


  while (1)
  {
	  static uint32_t last_check = 0;
	  static uint32_t led_off_time = 0;

// TESTING THE ENVELOPE PRESET

//	  static uint32_t last_gate_flip = 0;
//	  static bool virtual_gate = false;
//
//	  if (HAL_GetTick() - last_gate_flip > 1000) { // Every 1 second
//	      virtual_gate = !virtual_gate;
//	      last_gate_flip = HAL_GetTick();
//
//	      if (virtual_gate) {
//	          // "Press Key" -> Moves state to ATTACK
//	          Envelope_Trigger(&myPipeline.envelope);
//	      } else {
//	          // "Release Key" -> Moves state to RELEASE
//	          Envelope_Release(&myPipeline.envelope);
//	      }
//	  }

	  if (new_uart_data) {
		  new_uart_data = false;

		  synth.input_mode = rx_buffer[0];
		  synth.preset_id  = rx_buffer[1];
		  synth.attack     = rx_buffer[2];
		  synth.release    = rx_buffer[3];
		  synth.time       = rx_buffer[4];
		  synth.feedback   = rx_buffer[5];
		  synth.resolution = rx_buffer[6];

		  myPipeline.source          = synth.input_mode;
		  myPipeline.active_preset   = synth.preset_id;

		  myPipeline.source = (synth.input_mode == 0) ? SOURCE_CV : SOURCE_MIC;

		  // scaling knob data
		  myPipeline.envelope.attack_rate  = (float)synth.attack / 255.0f;
		  myPipeline.envelope.release_rate = (float)synth.release / 255.0f;
		  myPipeline.echo.time             = (float)synth.time / 255.0f;
		  myPipeline.echo.feedback         = (float)synth.feedback / 255.0f;

		  // (Assuming you add resolution to your pipeline later for the bitcrusher)
		  // myPipeline.bitcrush_res       = synth.resolution;

		  // debug info
		  char msg[100];
		  sprintf(msg, "Mode: %d | Pre: %d | A:%d REL:%d T:%d F:%d RES: %d\r\n",
				synth.input_mode, synth.preset_id, synth.attack,
				synth.release, synth.time, synth.feedback, synth.resolution);
		  Debug_Log(msg);
	  }

	  if (blink_led) {
	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	      led_off_time = HAL_GetTick() + 50;
	      blink_led = false;
	  }
	  if (led_off_time > 0 && HAL_GetTick() >= led_off_time) {
	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
	      led_off_time = 0;
	  }

	  // -------------------------------------------------------
	  // 2. HIGH-SPEED AUDIO ENGINE (Runs at 8kHz)
	  // -------------------------------------------------------
	  static uint32_t last_heartbeat = 0;
	  if (HAL_GetTick() - last_heartbeat > 1000) {
		  // This is safe because it only blocks the CPU once a second
		  HAL_UART_Transmit(&huart2, (uint8_t*)"ALIVE\r\n", 7, 10);
		  last_heartbeat = HAL_GetTick();
	  }

	  if (audio_ready) {
	      audio_ready = false;
	      float current_sample = 0.0f;

	      if (myPipeline.source == SOURCE_MIC) {
	          current_sample = Mic_ReadSample();
	      }
	      else {
	          current_sample = ((float)adc_val - 2048.0f) / 2048.0f;
	      }

	      bool gate_state = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);
	      float output = Pipeline_Process(&myPipeline, current_sample, gate_state);

	      // Output to PWM
	      uint32_t pwm_val = (uint32_t)((output + 1.0f) * 524.5f);
	      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (pwm_val > 1049) ? 1049 : pwm_val);
	  }
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}


// This function fires automatically exactly 8,000 times a second
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        audio_ready = true; // Tell the main loop to process the next sample!
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
    	blink_led = true;
        new_uart_data = true;
        HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
    }
}

// OLD CALLBACK
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//	if (huart->Instance == USART1) { // Change to your specific UART instance
//		blink_led = true;
//		synth.input_mode = rx_buffer[0];
//		synth.preset_id  = rx_buffer[1];
//		synth.attack     = rx_buffer[2];
//		synth.release    = rx_buffer[3];
//		synth.time       = rx_buffer[4];
//		synth.feedback   = rx_buffer[5];
//		synth.resolution = rx_buffer[6];
//
//		myPipeline.source          = synth.input_mode;
//		myPipeline.active_preset   = synth.preset_id;
//
//		// scaling knob data
//		myPipeline.envelope.attack_rate  = (float)synth.attack / 255.0f;
//		myPipeline.envelope.release_rate = (float)synth.release / 255.0f;
//		myPipeline.echo.time             = (float)synth.time / 255.0f;
//		myPipeline.echo.feedback         = (float)synth.feedback / 255.0f;
//
//		// (Assuming you add resolution to your pipeline later for the bitcrusher)
//		// myPipeline.bitcrush_res       = synth.resolution;
//
////		// 3. Print debug info
//		char msg[100];
//		sprintf(msg, "Mode: %d | Pre: %d | A:%d REL:%d T:%d F:%d RES: %d\r\n",
//				synth.input_mode, synth.preset_id, synth.attack,
//				synth.release, synth.time, synth.feedback, synth.resolution);
//		Debug_Log(msg);
//
//		// RE-ARM
//		HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
//	}
//}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        adc_val = HAL_ADC_GetValue(hadc);
        adc_ready_flag = 1; // Signal the main loop that data is ready
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Clear all error flags
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);

        // Re-arm so we can try again
        HAL_UART_Receive_IT(&huart1, rx_buffer, 7);
    }
}
/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  printf("Wrong parameters value: file %s on line %d\r\n", file, line)
}
#endif /* USE_FULL_ASSERT */
