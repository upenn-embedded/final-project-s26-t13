#include "interface.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "usart.h"

extern UART_HandleTypeDef huart2;
extern uint16_t knob_raw[5];

InterfaceState_t iface_state = {0};

void Interface_Init(void) {

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	for(int i=0; i<9; i++) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
		HAL_Delay(1);
	}

	Debug_Log("--- Interface Online: BUSY Flag Cleared ---");
    Debug_Log("Controls: PA9-PA12 + PC9 (Presets), 5 ADC Knobs");
}

void Interface_Update(void) {
    static uint32_t last_tick = 0;

    // Memory for button edges (presets 1-5)
    static GPIO_PinState last_p9 = GPIO_PIN_RESET;
    static GPIO_PinState last_p10 = GPIO_PIN_RESET;
    static GPIO_PinState last_p11 = GPIO_PIN_RESET;
    static GPIO_PinState last_p12 = GPIO_PIN_RESET;
    static GPIO_PinState last_pc9 = GPIO_PIN_RESET;


    // Memory for knob changes - MUST be static to persist between calls!
    static uint16_t last_knobs[5] = {0};
    static int threshold = 150;

    // 1. Timer Check (Run every 50ms)
    if (HAL_GetTick() - last_tick < 50) return;
    last_tick = HAL_GetTick();

    /* --- KNOB LOGIC --- */
    for (int i = 0; i < 5; i++) {
        int diff = abs((int)knob_raw[i] - (int)last_knobs[i]);
        int knob_num = i+1;
        if (diff > threshold) {
            iface_state.knobs[i] = knob_raw[i];
            last_knobs[i] = knob_raw[i];
            iface_state.dirty_flag = 1;

            // UART Debug for Knobs
            char buf[32];
            sprintf(buf, "KNOB %d: %d", knob_num, knob_raw[i]);
            Debug_Log(buf);
        }
    }

    /* --- BUTTON LOGIC --- */
    GPIO_PinState curr_p9  = HAL_GPIO_ReadPin(GPIOA, preset_1_Pin);
    GPIO_PinState curr_p10 = HAL_GPIO_ReadPin(GPIOB, preset_2_Pin);
    GPIO_PinState curr_p11 = HAL_GPIO_ReadPin(GPIOA, preset_3_Pin);
    GPIO_PinState curr_p12 = HAL_GPIO_ReadPin(GPIOA, preset_4_Pin);
    GPIO_PinState curr_pc9 = HAL_GPIO_ReadPin(GPIOC, preset_5_Pin);


    // PRESET 1
    if (curr_p9 == GPIO_PIN_SET && last_p9 == GPIO_PIN_RESET) {
        iface_state.preset_id = 1;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 1 Active");
    }
    last_p9 = curr_p9;

    // PRESET 2
    if (curr_p10 == GPIO_PIN_SET && last_p10 == GPIO_PIN_RESET) {
        iface_state.preset_id = 2;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 2 Active");
    }
    last_p10 = curr_p10;

    // PRESET 3
    if (curr_p11 == GPIO_PIN_SET && last_p11 == GPIO_PIN_RESET) {
        iface_state.preset_id = 3;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 3 Active");
    }
    last_p11 = curr_p11;

    // PRESET 4
    if (curr_p12 == GPIO_PIN_SET && last_p12 == GPIO_PIN_RESET) {
        iface_state.preset_id = 4;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 4 Active");
    }
    last_p12 = curr_p12;

    // PRESET 5
    if (curr_pc9 == GPIO_PIN_SET && last_pc9 == GPIO_PIN_RESET) {
		iface_state.preset_id = 5;
		iface_state.dirty_flag = 1;
		Debug_Log("BUTTON: Preset 5 Active");
	}
	last_pc9 = curr_pc9;

//        iface_state.dirty_flag = 0;
    if (iface_state.dirty_flag) {
        uint8_t uart_packet[7];
        uart_packet[0] = iface_state.preset_id;
        for(int i = 0; i < 5; i++) {
            uart_packet[i+1] = (uint8_t)(knob_raw[i] >> 4);
        }

        // Send via UART1 (assuming you enabled it in CubeMX)
        HAL_UART_Transmit(&huart1, uart_packet, 6, 10);
        if (HAL_UART_Transmit(&huart1, uart_packet, 6, 10) == HAL_OK) {
        	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
        	HAL_Delay(50);
        	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
            Debug_Log("UART Sent");

        }
        HAL_Delay(30);

        iface_state.dirty_flag = 0;
    }
}
