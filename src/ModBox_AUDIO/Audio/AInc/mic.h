/*
 * mic.h
 * Function: Handles hardware-level microphone sampling and normalization.
 */

#ifndef INC_MIC_H_
#define INC_MIC_H_

#include "main.h"

/**
 * @brief Reads a single sample from the Microphone ADC and normalizes it.
 * @retval float: A value between -1.0 and 1.0, where 0.0 is silence.
 */
float Mic_ReadSample(void);

/**
 * @brief (Optional) Calibrates the mic offset if your silence isn't 2048.
 */
void Mic_Calibrate(void);

#endif /* INC_MIC_H_ */
