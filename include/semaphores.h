#pragma once

#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>

namespace Semaphores
{
    extern SemaphoreHandle_t displayPomodoroHandshakeSemaphore;
    extern SemaphoreHandle_t pirEventSemaphore;

    void vCreateSemaphores(void);
} // namespace Semaphores
