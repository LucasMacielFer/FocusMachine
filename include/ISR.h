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
    void setupISRs();
    void attachPIRISR();
    void detachPIRISR();
    void IRAM_ATTR pirISR();
    void IRAM_ATTR buttonPressISR();
    void IRAM_ATTR touchISR(void* arg);
} // namespace ISR