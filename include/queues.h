#pragma once

#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>

#include "types.h"

namespace Queues
{
    // Idel task hook -> Telemetry task
    extern QueueHandle_t sysMonitorQueue;

    // Telemetry Task/Pomodoro FSM Task -> Display task
    extern QueueHandle_t displayQueue;

    // DHT sensor task -> Telemetry task
    extern QueueHandle_t dhtSensorQueue;

    // PIR sensor task -> Telemetry task
    extern QueueHandle_t pirSensorQueue;

    // LDR sensor task -> Telemetry task
    extern QueueHandle_t ldrSensorQueue;

    // Camera inference task -> Telemetry task
    extern QueueHandle_t cameraInferenceQueue;

    // Interaction ISRs -> Pomodoro FSM task
    extern QueueHandle_t interactionEventQueue;

    // Pomodoro FSM task -> Telemetry task
    extern QueueHandle_t systemStateQueue;

    void vCreateQueues(void);
} // namespace Queues