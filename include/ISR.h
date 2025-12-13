#pragma once
#include <Arduino.h>
#include "pins.h"
#include "queues.h"
#include "semaphores.h"

#define FROM_BUTTON 0
#define FROM_TOUCH1 1
#define FROM_TOUCH2 2

#define TOUCH_THRESHOLD_1 1800
#define TOUCH_THRESHOLD_2 6700
#define DEBOUNCE_INTERVAL_MS 400

namespace ISR
{
    // Function to setup all ISRs
    void setupISRs();

    // PIR ISR attach/detach functions
    void attachPIRISR();
    void detachPIRISR();

    // ISR called upon PIR detection
    void IRAM_ATTR pirISR();
    // ISR called upon button press
    void IRAM_ATTR buttonPressISR();
    // ISR called upon touch1/touch2 events
    void IRAM_ATTR touchISR(void* arg);
} // namespace ISR