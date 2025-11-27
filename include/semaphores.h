#pragma once

#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>

namespace Semaphores
{
    extern SemaphoreHandle_t sysMonitorSemaphore;

    void vCreateSemaphores(void);
} // namespace Semaphores
