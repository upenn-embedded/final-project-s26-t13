/* main.c
 * Embedded modular synthesizer — STM32F4
 *
 * INTEGRATION SUMMARY
 * -------------------
 * 1. AudioPipeline_t pipeline  — global instance, inited in main().
 * 2. Pipeline_ApplyParams()    — called whenever a UART packet arrives,
 *                                propagates all 7 parameters into the pipeline.
 * 3. Pipeline_Process()        — called in the audio loop for every new IC
 *                                capture; its float output drives the PWM.
 *
 * Signal flow (per preset — see audio_pipeline.c preset_table[]):
 *   ADC/IC input → Pipeline_Process() → PWM compare register (PA5)
 */

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "audio_pipeline.h"    /* <-- pipeline header */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * Synth parameter packet (7 bytes over UART)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint8_t input_mode;   /* byte 0: 0 = CV */
    uint8_t preset_id;    /* byte 1: 0-3 selects module chain */
    uint8_t attack;       /* byte 2: 0-255 → envelope attack rate */
    uint8_t release;      /* byte 3: 0-255 → envelope release rate */
    uint8_t time;         /* byte 4: 0-255 → echo delay time */
    uint8_t feedback;     /* byte 5: 0-255 → echo feedback */
} SynthParams_t;

/* --------------------------------------------------------------------------
 * Globals
 * -------------------------------------------------------------------------- */
SynthParams_t    synth          = {0};
AudioPipeline_t  pipeline;                  /* <-- pipeline instance */

uint8_t          rx_buffer[7];
volatile bool    new_data_flag  = false;

volatile uint32_t ic_period      = 0;
volatile uint32_t ic_last_capture = 0;
volatile bool     ic_valid        = false;
volatile bool     ic_updated      = false;
volatile bool     ic_first        = true;

/* Last ADC reading — updated every ic_updated tick, read by debug print */
static uint32_t  last_adc_raw   = 0;

/* --------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------- */
void SystemClock_Config(void);
void Debug_Log(const char *msg);

/* --------------------------------------------------------------------------
 * main()
 * -------------------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();          /* Debug UART                              */
    MX_USART1_UART_Init();          /* Interface UART (7-byte packets)         */
    MX_ADC1_Init();
    MX_TIM2_Init();                 /* PWM pitch output on PA5                 */
    MX_TIM3_Init();                 /* Input capture on PA6 (pitch tracking)   */

    /* Start peripherals */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 525);

    HAL_ADC_Start(&hadc1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

    /* Start UART DMA receive */
    if (HAL_UART_Receive_DMA(&huart1, rx_buffer, 7) == HAL_OK) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    /* -----------------------------------------------------------------------
     * Initialise the audio pipeline.
     * This sets default module parameters; they will be overwritten by the
     * first UART packet via Pipeline_ApplyParams() below.
     * --------------------------------------------------------------------- */
    Pipeline_Init(&pipeline);

    Debug_Log("SYSTEM STARTING...");

    while (1)
    {
        /* -------------------------------------------------------------------
         * 1. AUDIO PROCESSING
         *
         * When a new input-capture period is available, read the raw ADC
         * value, run it through the pipeline (which applies the active preset
         * module chain), and write the processed result to the PWM register.
         * ----------------------------------------------------------------- */
        if (ic_valid && ic_updated)
        {
            ic_updated = false;

            /* Compute the new PWM period from the captured IC frequency */
            uint32_t dynamic_period = (uint32_t)(8.4f * (float)ic_period) - 1;

            /* Clamp to 200 Hz – 5 kHz */
            if (dynamic_period > 41999) dynamic_period = 41999;
            if (dynamic_period < 1679)  dynamic_period = 1679;

            /* -----------------------------------------------------------
             * Read raw ADC sample and determine hardware gate state.
             *
             * ADC value (0-4095) is normalised to a signed 16-bit range
             * so it is compatible with the pipeline's int16_t modules.
             * --------------------------------------------------------- */
            HAL_ADC_PollForConversion(&hadc1, 1);
            uint32_t adc_raw      = HAL_ADC_GetValue(&hadc1);
            last_adc_raw          = adc_raw;   /* expose to debug print */
            float    input_sample = (float)((int32_t)adc_raw - 2048) * 16.0f;

            /* Gate: treat upper half of ADC range as "gate open".
             * Replace with your actual gate GPIO read if available. */
            bool hardware_gate = (adc_raw > 2048);

            /* -----------------------------------------------------------
             * Run the sample through the active preset module chain.
             * pipeline.active_preset is set by Pipeline_ApplyParams()
             * whenever a new UART packet arrives.
             * --------------------------------------------------------- */
            float processed = Pipeline_Process(&pipeline, input_sample, hardware_gate);

            /* -----------------------------------------------------------
             * Map processed float back to a PWM compare value.
             *
             * processed is roughly in the range [-32768, 32767].
             * We shift it into [0, dynamic_period] for the PWM register.
             * --------------------------------------------------------- */
            int32_t pwm_val = (int32_t)(processed / 32768.0f
                              * (float)(dynamic_period / 2))
                              + (int32_t)(dynamic_period / 2);

            /* Clamp to valid compare range */
            if (pwm_val < 0)                        pwm_val = 0;
            if (pwm_val > (int32_t)dynamic_period)  pwm_val = (int32_t)dynamic_period;

            /* Write new period and duty cycle to TIM2 */
            TIM2->CR1 &= ~TIM_CR1_ARPE;
            __HAL_TIM_SET_AUTORELOAD(&htim2, dynamic_period);

            if (__HAL_TIM_GET_COUNTER(&htim2) >= dynamic_period)
                __HAL_TIM_SET_COUNTER(&htim2, 0);

            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (uint32_t)pwm_val);
        }

        /* -------------------------------------------------------------------
         * 2. PARAMETER SYNC
         *
         * When the UART DMA callback sets new_data_flag, push the fresh
         * packet into the pipeline.  Pipeline_ApplyParams() scales the raw
         * bytes to the correct ranges for every module and switches presets.
         * ----------------------------------------------------------------- */
        if (new_data_flag)
        {
            new_data_flag = false;

            /* Cache locally for atomic read */
            synth.input_mode = rx_buffer[0];
            synth.preset_id  = rx_buffer[1];
            synth.attack     = rx_buffer[2];
            synth.release    = rx_buffer[3];
            synth.time       = rx_buffer[4];
            synth.feedback   = rx_buffer[5];

            /* Apply all parameters to the pipeline in one call */
            Pipeline_ApplyParams(&pipeline,
                                 synth.input_mode,
                                 synth.preset_id,
                                 synth.attack,
                                 synth.release,
                                 synth.time,
                                 synth.feedback);

            /* Debug: print preset name and all raw parameter values */
            static const char *preset_names[] = {
                "0: CLEAN",
                "1: WARM ECHO",
                "2: LO-FI ECHO",
                "3: LONG TAIL"
            };
            const char *pname = (synth.preset_id < 4)
                                 ? preset_names[synth.preset_id]
                                 : "?: UNKNOWN";

            char dbg[80];
            uint32_t cv_mv = (last_adc_raw * 3300UL) / 4095UL;  /* raw → mV */
            snprintf(dbg, sizeof(dbg),
                     "[PRESET %s] CV:%4lumV SRC:%d ATK:%3d REL:%3d TIME:%3d FB:%3d",
                     pname,
                     (unsigned long)cv_mv,
                     synth.input_mode,
                     synth.attack,
                     synth.release,
                     synth.time,
                     synth.feedback);
            Debug_Log(dbg);
        }
    }
}

/* --------------------------------------------------------------------------
 * HAL Callbacks
 * -------------------------------------------------------------------------- */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        uint32_t current = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        if (!ic_first)
        {
            if (current >= ic_last_capture)
                ic_period = current - ic_last_capture;
            else
                ic_period = (0xFFFF - ic_last_capture) + current + 1;

            ic_valid   = true;
            ic_updated = true;
        }

        ic_first        = false;
        ic_last_capture = current;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* NOTE: Pipeline_ApplyParams() is NOT called here — it runs from
         * the main loop once new_data_flag is checked.  This keeps the ISR
         * short and avoids calling floating-point math from interrupt context. */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
        new_data_flag = true;

        /* Restart DMA for the next packet */
        HAL_UART_Receive_DMA(&huart1, rx_buffer, 7);
    }
}

/* --------------------------------------------------------------------------
 * Utilities
 * -------------------------------------------------------------------------- */

void Debug_Log(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 10);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 10);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM           = 16;
    RCC_OscInitStruct.PLL.PLLN           = 336;
    RCC_OscInitStruct.PLL.PLLP           = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ           = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}
