#include "interface.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;
extern uint16_t knob_raw[5];

InterfaceState_t iface_state = {0, 0, 0};


void Interface_Init(void) {
    Debug_Log("--- Interface Debugger Online ---");
    Debug_Log("Controls: PC13 (Toggle Mode), PA5-PA8 (Presets), 5 ADC Knobs");
}

void Interface_Update(void) {
	static uint32_t last_tick = 0;
	static GPIO_PinState last_input_btn_state = GPIO_PIN_SET;
	static GPIO_PinState last_p9 = GPIO_PIN_SET;
	static GPIO_PinState last_p10 = GPIO_PIN_SET;
	static GPIO_PinState last_p11 = GPIO_PIN_SET;
	static GPIO_PinState last_p12 = GPIO_PIN_SET;

	uint16_t last_knobs[5] = {0, 0, 0, 0, 0};

	// debounce
	if (HAL_GetTick() - last_tick < 50) return;
	last_tick = HAL_GetTick();

	// states read
	GPIO_PinState current_btn_state = HAL_GPIO_ReadPin(GPIOC, input_type_Pin);
	GPIO_PinState curr_p9  = HAL_GPIO_ReadPin(GPIOA, preset_1_Pin);
	GPIO_PinState curr_p10 = HAL_GPIO_ReadPin(GPIOA, preset_2_Pin);
	GPIO_PinState curr_p11 = HAL_GPIO_ReadPin(GPIOA, preset_3_Pin);
	GPIO_PinState curr_p12 = HAL_GPIO_ReadPin(GPIOA, preset_4_Pin);


	// INPUT BUTTON
	if (current_btn_state == GPIO_PIN_SET && last_input_btn_state == GPIO_PIN_RESET)
	{
	    iface_state.input_mode = (iface_state.input_mode == 0) ? 1 : 0;

	    iface_state.dirty_flag = 1;

	    if (iface_state.input_mode == 0) {
	        Debug_Log("INPUT MODE: [0] Control Voltage");
	    } else {
	        Debug_Log("INPUT MODE: [1] Microphone");
	    }
	}
	last_input_btn_state = current_btn_state;

	// Preset 1 (PA9)
	if (curr_p9 == GPIO_PIN_SET && last_p9 == GPIO_PIN_RESET) {
		iface_state.preset_id = 0;
		iface_state.dirty_flag = 1;
		Debug_Log("BUTTON: Preset 1 Active");
	}
	last_p9 = curr_p9;

	// Preset 2
	if (curr_p10 == GPIO_PIN_SET && last_p10 == GPIO_PIN_RESET) {
		iface_state.preset_id = 1;
		iface_state.dirty_flag = 1;
		Debug_Log("BUTTON: Preset 2 Active");
	}
	last_p10 = curr_p10;

	// Preset 3 (PA11)
	if (curr_p11 == GPIO_PIN_SET && last_p11 == GPIO_PIN_RESET) {
		iface_state.preset_id = 2;
		iface_state.dirty_flag = 1;
		Debug_Log("BUTTON: Preset 3 Active");
	}
	last_p11 = curr_p11;

	// Preset 4 (PA12)
	if (curr_p12 == GPIO_PIN_SET && last_p12 == GPIO_PIN_RESET) {
		iface_state.preset_id = 3;
		iface_state.dirty_flag = 1;
		Debug_Log("BUTTON: Preset 4 Active");
	}
	last_p12 = curr_p12;

	// 4. Send I2C if any button was pressed
	if (iface_state.dirty_flag) {
		iface_state.dirty_flag = 0;
		uint8_t i2c_packet[7];
		// ... packet assembly ...
		HAL_I2C_Master_Transmit(&hi2c1, (0x12 << 1), i2c_packet, 7, 10);
	}

    // 4. Knob Change Detection
    for(int i = 0; i < 5; i++) {
        // Threshold of 30 prevents "jitter" from triggering I2C
        if (abs((int)knob_raw[i] - (int)last_knobs[i]) > 150) {
            iface_state.dirty_flag = 1;
            last_knobs[i] = knob_raw[i];

            // Debug specific knob movements
//            sprintf(dbg_buf, "Knob %d moved: %d", i, knob_raw[i]);
//            Debug_Log(dbg_buf);
        }
    }

    // 5. I2C Transmission (The Dirty Flag Check)
    if (iface_state.dirty_flag) {
        /* Packet Structure (7 Bytes):
           [0] = Source (0/1)
           [1] = Preset ID (0-3)
           [2] = Attack (Scaled 0-255)
           [3] = Release
           [4] = Echo Time
           [5] = Echo Feedback
           [6] = Resolution
        */
        uint8_t i2c_packet[7];
        i2c_packet[0] = iface_state.input_mode;
        i2c_packet[1] = iface_state.preset_id;

        for(int i = 0; i < 5; i++) {
            // Bit-shift 12-bit (4095) to 8-bit (255)
            i2c_packet[i+2] = (uint8_t)(knob_raw[i] >> 4);
        }

        // Target Audio Board Address: 0x12
        // Shift address left by 1 for HAL (7-bit to 8-bit write address)
//        if (HAL_I2C_Master_Transmit(&hi2c1, (0x12 << 1), i2c_packet, 7, 5) != HAL_OK) {
//			// Only log the error once per change
//			Debug_Log("[I2C] Offline (No Audio Board)");
//		}

		// MOVE THIS HERE: Reset the flag regardless of success for now
		iface_state.dirty_flag = 0;
    }
}
