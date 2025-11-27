#pragma once
#include <Arduino.h>
#include "pins.h"
#include "queues.h"
#include "semaphores.h"

#define FROM_BUTTON 0
#define FROM_TOUCH1 1
#define FROM_TOUCH2 2

namespace ISR
{
    void setupISRs();
    void attachPIRISR();
    void detachPIRISR();
    void IRAM_ATTR pirISR();
    void IRAM_ATTR buttonPressISR();
    void IRAM_ATTR touchISR(void* arg);
} // namespace ISR