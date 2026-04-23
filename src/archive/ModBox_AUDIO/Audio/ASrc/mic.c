#include "mic.h"
#include "adc.h"

// We store the "Silence" level. Usually 2048 for a 12-bit ADC,
// but we can adjust this if the hardware has a slight offset.
#define MIC_OFFSET 2048.0f

float Mic_ReadSample(void) {
    uint32_t raw_val = 0;

    // 1. Trigger the ADC
    HAL_ADC_Start(&hadc1);

    // 2. Wait for the conversion (Polling is okay here as it's very fast)
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
        raw_val = HAL_ADC_GetValue(&hadc1);
    }

    // 3. Convert to Bipolar Float (-1.0 to 1.0)
    // We subtract the offset so that silence = 0.0
    float normalized = ((float)raw_val - MIC_OFFSET) / MIC_OFFSET;

    // 4. Bound the output (Safety first!)
    if (normalized > 1.0f)  normalized = 1.0f;
    if (normalized < -1.0f) normalized = -1.0f;

    return normalized;
}
