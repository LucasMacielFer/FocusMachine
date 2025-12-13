#include <Arduino.h>
#include "tasks.h"
#include "cam_setup.h"
#include "ISR.h"

void setup() 
{
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
    Serial.begin(115200);

    // Initialize camera
    sensor_t * s = Camera::startCamera();

    // Disable camera and GPIO logs -> overflow-prone
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("camera", ESP_LOG_NONE);
    esp_log_level_set("cam_hal", ESP_LOG_NONE);

    Pins::initializePins();
    Semaphores::vCreateSemaphores();
    Queues::vCreateQueues();
    SystemSync::initializeSystemSync();
    Tasks::vTasksInitialize();
    ISR::setupISRs();

    interrupts();
}

void loop() 
{
    vTaskDelete(NULL);
}