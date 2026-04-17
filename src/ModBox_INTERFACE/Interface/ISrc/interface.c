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
    Debug_Log("Controls: PC13 (Toggle Mode), PA9-PA12 (Presets), 5 ADC Knobs");
}

void Interface_Update(void) {
    static uint32_t last_tick = 0;

    // Memory for button edges
    static GPIO_PinState last_input_btn_state = GPIO_PIN_RESET;
    static GPIO_PinState last_p9 = GPIO_PIN_RESET;
    static GPIO_PinState last_p10 = GPIO_PIN_RESET;
    static GPIO_PinState last_p11 = GPIO_PIN_RESET;
    static GPIO_PinState last_p12 = GPIO_PIN_RESET;

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
    GPIO_PinState current_btn_state = HAL_GPIO_ReadPin(input_type_GPIO_Port, input_type_Pin);
    GPIO_PinState curr_p9  = HAL_GPIO_ReadPin(GPIOA, preset_1_Pin);
    GPIO_PinState curr_p10 = HAL_GPIO_ReadPin(GPIOB, preset_2_Pin);
    GPIO_PinState curr_p11 = HAL_GPIO_ReadPin(GPIOA, preset_3_Pin);
    GPIO_PinState curr_p12 = HAL_GPIO_ReadPin(GPIOA, preset_4_Pin);

    // INPUT MODE TOGGLE (PC9)
    if (current_btn_state == GPIO_PIN_SET && last_input_btn_state == GPIO_PIN_RESET) {
        iface_state.input_mode = (iface_state.input_mode == 0) ? 1 : 0;
        iface_state.dirty_flag = 1;
        Debug_Log(iface_state.input_mode == 0 ? "MODE: Control Voltage" : "MODE: Microphone");
    }
    last_input_btn_state = current_btn_state;

    // PRESET 1
    if (curr_p9 == GPIO_PIN_SET && last_p9 == GPIO_PIN_RESET) {
        iface_state.preset_id = 0;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 1 Active");
    }
    last_p9 = curr_p9;

    // PRESET 2
    if (curr_p10 == GPIO_PIN_SET && last_p10 == GPIO_PIN_RESET) {
        iface_state.preset_id = 1;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 2 Active");
    }
    last_p10 = curr_p10;

    // PRESET 3
    if (curr_p11 == GPIO_PIN_SET && last_p11 == GPIO_PIN_RESET) {
        iface_state.preset_id = 2;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 3 Active");
    }
    last_p11 = curr_p11;

    // PRESET 4
    if (curr_p12 == GPIO_PIN_SET && last_p12 == GPIO_PIN_RESET) {
        iface_state.preset_id = 3;
        iface_state.dirty_flag = 1;
        Debug_Log("BUTTON: Preset 4 Active");
    }
    last_p12 = curr_p12;

    /* --- I2C TRANSMISSION --- */
//    if (iface_state.dirty_flag) {
//        uint8_t i2c_packet[7];
//        i2c_packet[0] = iface_state.input_mode;
//        i2c_packet[1] = iface_state.preset_id;
//
//        for(int i = 0; i < 5; i++) {
//            // Scale 12-bit to 8-bit for the Audio STM
//            i2c_packet[i+2] = (uint8_t)(knob_raw[i] >> 4);
//        }
//
//        // Transmit to Audio Board (0x12)
//        if (HAL_I2C_Master_Transmit(&hi2c1, (0x12 << 1), i2c_packet, 7, 5) == HAL_OK) {
//        }
//	}
//
//        iface_state.dirty_flag = 0;
    if (iface_state.dirty_flag) {
        uint8_t uart_packet[7];
        uart_packet[0] = iface_state.input_mode;
        uart_packet[1] = iface_state.preset_id;
        for(int i = 0; i < 5; i++) {
            uart_packet[i+2] = (uint8_t)(knob_raw[i] >> 4);
        }

        // Send via UART1 (assuming you enabled it in CubeMX)
        HAL_UART_Transmit(&huart1, uart_packet, 7, 10);

        iface_state.dirty_flag = 0;
        Debug_Log("UART Sent");
    }
}
