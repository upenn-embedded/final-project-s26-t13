#include "interface.h"
#include <stdio.h>
#include <string.h>

// Bring in the handles from main.c
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

InterfaceState_t iface_state = {0, 0, 0};

void Debug_Log(const char* msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
}

void Interface_Init(void) {
    Debug_Log("--- Interface Debugger Online ---");
    Debug_Log("Testing Pins: PC13 (Toggle), PA5-PA8 (Presets)");
}

void Interface_Update(void) {
    static uint32_t last_tick = 0;
    char dbg_buf[50];

    // 1. Debounce: Only check every 50ms
    if (HAL_GetTick() - last_tick < 50) return;
    last_tick = HAL_GetTick();

    // 2. Check Input Toggle (PC13) - Assuming Active Low (Internal Pull-up)
    uint8_t current_mode = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) ? 1 : 0;
    if (current_mode != iface_state.input_mode) {
        iface_state.input_mode = current_mode;
        sprintf(dbg_buf, "[DEBUG] PC13 Triggered: Mode is now %s", (current_mode ? "MIC" : "CV"));
        Debug_Log(dbg_buf);
        iface_state.dirty_flag = 1;
    }

    // 3. Check Presets (PA5-PA8) - Assuming Active High
    uint8_t active_p = iface_state.preset_id;
    if      (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET) active_p = 0;
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET) active_p = 1;
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET) active_p = 2;
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET) active_p = 3;

    if (active_p != iface_state.preset_id) {
        iface_state.preset_id = active_p;
        sprintf(dbg_buf, "[DEBUG] Preset Pin Triggered: Selected %d", active_p + 1);
        Debug_Log(dbg_buf);
        iface_state.dirty_flag = 1;
    }

    // 4. Placeholder for I2C Transmit (We'll add this once buttons work)
    if (iface_state.dirty_flag) {
        // HAL_I2C_Master_Transmit(...);
        iface_state.dirty_flag = 0;
    }
}
