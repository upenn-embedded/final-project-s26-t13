#include "main.h"
#include "echo.h"

void Echo_Init(Echo_t* echo, uint32_t delay_ms) {
    for(int i=0; i < ECHO_BUFFER_SIZE; i++) echo->buffer[i] = 0.0f;
    echo->write_pos = 0;
    echo->feedback = 0.5f;
    echo->time = 0.5f;
    echo->mix = 0.4f;
    // Calculate delay based on 8kHz sample rate
    echo->delay_samples = (delay_ms * 8);
    if(echo->delay_samples >= ECHO_BUFFER_SIZE) echo->delay_samples = ECHO_BUFFER_SIZE - 1;
}

float Echo_Process(Echo_t* echo, float input) {
    // 1. Calculate where to read from the past
    int32_t read_pos = echo->write_pos - echo->delay_samples;
    if (read_pos < 0) read_pos += ECHO_BUFFER_SIZE;

    float delayed_sample = echo->buffer[read_pos];

    // 2. Write new input + feedback into the buffer
    echo->buffer[echo->write_pos] = input + (delayed_sample * echo->feedback);

    // 3. Increment write position
    echo->write_pos = (echo->write_pos + 1) % ECHO_BUFFER_SIZE;

    // 4. Mix Dry and Wet signals
    return (input * (1.0f - echo->mix)) + (delayed_sample * echo->mix);
}
