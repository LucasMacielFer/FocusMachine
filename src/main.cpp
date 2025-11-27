#include <Arduino.h>
#include "tasks.h"
#include "cam_setup.h"
#include "ISR.h"

void setup() 
{
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
    Serial.begin(115200);

    sensor_t * s = Camera::startCamera();

    Pins::initializePins();
    Semaphores::vCreateSemaphores();
    Queues::vCreateQueues();
    Tasks::vTasksInitialize();
    ISR::setupISRs();

    interrupts();
}

void loop() 
{
    vTaskDelete(NULL);
}