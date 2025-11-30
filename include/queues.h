#pragma once

#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>

#include "types.h"

namespace Queues
{
    extern QueueHandle_t sysMonitorQueue;
    extern QueueHandle_t displayQueue;
    extern QueueHandle_t dhtSensorQueue;
    extern QueueHandle_t pirSensorQueue;
    extern QueueHandle_t ldrSensorQueue;
    extern QueueHandle_t cameraInferenceQueue;
    extern QueueHandle_t interactionEventQueue;
    extern QueueHandle_t systemStateQueue;

    void vCreateQueues(void);
} // namespace Queues