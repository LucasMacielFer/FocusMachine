#pragma once

#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>

namespace Semaphores
{
    /* Hanshake semaphore between display and pomodoro: Pomodoro can only 
    *  start executing a new state after the display has updated accordingly.
    */
    extern SemaphoreHandle_t displayPomodoroHandshakeSemaphore;

    // Event-counter semaphore for PIR sensor detections
    extern SemaphoreHandle_t pirEventSemaphore;

    // Mutex to protect the Serial port
    extern SemaphoreHandle_t serialMutex;

    // Function to create all the semaphores
    void vCreateSemaphores(void);
} // namespace Semaphores
