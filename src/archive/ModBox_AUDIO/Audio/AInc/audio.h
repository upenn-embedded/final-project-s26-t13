#ifndef AUDIOMAIN_H
#define AUDIOMAIN_H

#include "main.h"            /* Includes HAL drivers and peripheral handles */
#include "audio_pipeline.h"  /* Includes our DSP logic (Envelope, Echo, etc) */

/* --- Configuration Constants --- */
#define SAMPLE_RATE          8000     /* 8kHz for vocal/lo-fi character */
#define DAC_MIDPOINT         2048     /* 12-bit DAC center for AC audio signals */

/* --- Global Pipeline State --- */
/* This allows the Main Loop to change presets while the Interrupt processes audio */

/**
 * @brief Initializes the audio peripherals and the DSP pipeline.
 * Called once at startup in main.c.
 */
void AudioSystem_Init(void);

/**
 * @brief The core hardware-to-software bridge.
 * Typically called from within the Timer Interrupt (TIM2) or a DMA callback.
 * 1. Reads ADC (VCO/Mic)
 * 2. Processes through Pipeline_Process
 * 3. Writes DAC1 (Audio to VCF)
 * 4. Writes DAC2 (Envelope CV to VCA)
 */
void AudioSystem_ProcessTick(void);

/**
 * @brief Handles background tasks for the audio MCU.
 * Reads the local preset/source pins (A0-A4) to update pipeline routing.
 */
void AudioSystem_HandleInputs(void);

void Run_Basic_PWM_Test(void);

void Audio_Internal_Test(void);
#endif /* AUDIOMAIN_H */
