#ifndef INTERFACE_H
#define INTERFACE_H

#include "main.h"

// State structure to track buttons
typedef struct {
    uint8_t input_mode;   // 0: CV, 1: MIC
    uint8_t preset_id;    // 0-3
    uint8_t dirty_flag;   // Set to 1 when a change occurs
} InterfaceState_t;

// This is the structure that will actually be sent over the wire
typedef struct {
    uint8_t source;     // 0 or 1
    uint8_t preset;     // 0-3
    uint16_t knobs[5];  // Attack, Release, Time, Feedback, Resolution
} SynthCommand_t;

// Function Prototypes
void Interface_Init(void);
void Interface_Update(void);
void Debug_Log(const char* msg);

#endif
