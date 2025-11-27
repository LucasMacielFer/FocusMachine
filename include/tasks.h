#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_freertos_hooks.h"

#include "semaphores.h"
#include "queues.h"
#include "types.h"
#include "pins.h"
#include "classification.h"
#include "display.h"

// Sensor lib
#include <DHT.h>

#define DHT_TASK_DELAY_MS 2000
#define PIR_TASK_DELAY_MS 500
#define LDR_TASK_DELAY_MS 500
#define CAMERA_TASK_DELAY_MS 200
#define POMODORO_TASK_DELAY_MS 1000

namespace Tasks 
{
    // Idle task tag
    extern uint16_t* pCounter;

    // Handle for the idle task
    extern TaskHandle_t xIdleTaskHandle;

    // Task initialization function
    void vTasksInitialize();

    // Task function declarations
    void vPomodoroFSMTask(void* parameter);
    void vBrainTask(void* parameter);
    void vDisplayTask(void* parameter);
    void vCameraInferenceTask(void* parameter);
    void vDHTSensorTask(void* parameter);
    void vPIRSensorTask(void* parameter);
    void vLDRSensorTask(void* parameter);
    void vBuzzerTask(void* parameter);

    // Hook function declarations
    bool vSysMonitorIdleHook();
}  // namespace Tasks