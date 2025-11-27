#include <Arduino.h>
#include "tasks.h"
#include "cam_setup.h"

void setup() 
{
  vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
  pinMode(PIN_PWM, OUTPUT);
  ledcAttachPin(PIN_PWM, 0);
  ledcSetup(0, 500, 8); // Channel 0, 500 hZ, 8-bit resolution
  ledcWrite(0, 0);
  Serial.begin(115200);
  sensor_t * s = Camera::startCamera();

  Pins::initializePins();
  Semaphores::vCreateSemaphores();
  Queues::vCreateQueues();
  Tasks::vTasksInitialize();
}

void loop() 
{
  vTaskDelete(NULL);
}