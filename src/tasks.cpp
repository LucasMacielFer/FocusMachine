#include "tasks.h"

namespace Tasks
{
    void vTasksInitialize()
    {
        //xTaskCreatePinnedToCore(vPomodoroFSMTask, "PomodoroFSM", 4096, NULL, 4, NULL, 0);
        xTaskCreatePinnedToCore(vBrainTask, "BrainTask", 8192, NULL, 3, NULL, 0);
        //xTaskCreatePinnedToCore(vDisplayTask, "DisplayTask", 8192, NULL, 2, NULL, 0);
        xTaskCreatePinnedToCore(vCameraInferenceTask, "CameraInferenceTask", 8192, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(vDHTSensorTask, "DHTSensorTask", 4096, NULL, 1, NULL, 0);
        //xTaskCreatePinnedToCore(vPIRSensorTask, "PIRSensorTask", 4096, NULL, 1, NULL, 0);
        //xTaskCreatePinnedToCore(vLDRSensorTask, "LDRSensorTask", 4096, NULL, 1, NULL, 0);

        esp_register_freertos_idle_hook(Tasks::vSysMonitorIdleHook);
    }

    void vPomodoroFSMTask(void* parameter)
    {
        return;
    }
    
    void vBrainTask(void* parameter)
    {
        // Variables to hold sensor data and system metrics
        Types::DHTSensorData dhtData;
        Types::SystemMetrics sysMetrics;
        float cpuUsage;
        int PIRdetections;
        int luminosity;
        bool faceDetected;

        float comfortIndex;

        QueueHandle_t dhtSensorQueue = Queues::dhtSensorQueue;
        QueueHandle_t sysMonitorQueue = Queues::sysMonitorQueue;
        QueueHandle_t pirSensorQueue = Queues::pirSensorQueue;
        QueueHandle_t ldrSensorQueue = Queues::ldrSensorQueue;
        QueueHandle_t displayQueue = Queues::displayQueue;
        QueueHandle_t cameraInferenceQueue = Queues::cameraInferenceQueue;

        // Queue set
        QueueSetHandle_t brainQueueSet = xQueueCreateSet(5); 

        xQueueAddToSet(dhtSensorQueue, brainQueueSet);
        xQueueAddToSet(sysMonitorQueue, brainQueueSet);
        xQueueAddToSet(pirSensorQueue, brainQueueSet);
        xQueueAddToSet(ldrSensorQueue, brainQueueSet);
        xQueueAddToSet(cameraInferenceQueue, brainQueueSet);

        QueueSetMemberHandle_t xActivatedMember;

        for(;;)
        {
            xActivatedMember = xQueueSelectFromSet(brainQueueSet, portMAX_DELAY);
            if (xActivatedMember == dhtSensorQueue) 
            {
                if (xQueueReceive(dhtSensorQueue, &dhtData, 0) == pdTRUE)
                {
                    Serial.print("Temperature: ");
                    Serial.print(dhtData.temperature);
                    Serial.print(" °C, Humidity: ");
                    Serial.print(dhtData.humidity);
                    Serial.println(" %");
                }
            }

            // Read system metrics
            if (xActivatedMember == sysMonitorQueue) 
            {
                if (xQueueReceive(sysMonitorQueue, &sysMetrics, 0) == pdTRUE)
                    if (sysMetrics.totalTicks > 0)
                    {
                        cpuUsage = 100.0f * (1.0f - ((float)sysMetrics.idleTicks / (float)sysMetrics.totalTicks));
                        
                        // Opcional: Proteger contra valores negativos (se o clock do RTOS der um salto louco)
                        if (cpuUsage < 0.0f) cpuUsage = 0.0f;
                        if (cpuUsage > 100.0f) cpuUsage = 100.0f;

                        Serial.print("CPU Usage: ");
                        Serial.print(cpuUsage);
                        Serial.println(" %");
                    }
                    else
                    {
                        Serial.println("Erro: TotalTicks é 0");
                    }
            }

            if (xActivatedMember == pirSensorQueue) 
            {
                if (xQueueReceive(pirSensorQueue, &PIRdetections, 0) == pdTRUE)
                {
                    Serial.print("PIR Detections: ");
                    Serial.println(PIRdetections);
                }
            }

            if (xActivatedMember == ldrSensorQueue) 
            {
                if (xQueueReceive(ldrSensorQueue, &luminosity, 0) == pdTRUE)
                {
                    Serial.print("Luminosity: ");
                    Serial.println(luminosity);
                }
            }

            if (xActivatedMember == cameraInferenceQueue) 
            {
                if (xQueueReceive(cameraInferenceQueue, &faceDetected, 0) == pdTRUE)
                {
                    Serial.println("Camera Inference Data Received");
                }
            }
        }
    }

    void vDisplayTask(void* parameter)
    {
        return;
    }

    void vCameraInferenceTask(void* parameter)
    {
        camera_fb_t * fb;
        float acc = 0.0;
        float count = 0;
        float cont2, cont1 = 0;
        bool detected = false;
        float taxa = 0.0;

        while (1)
        {
            fb = esp_camera_fb_get();
            detected = faceDetect(fb, &acc);
            if(detected) 
            {
                Serial.printf("ROSTO DETECTADO! Precisão: %.2f%%\n", acc * 100);
            }
            else 
            {
                Serial.println("Nenhum rosto...");
            }

            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(CAMERA_TASK_DELAY_MS));
        }
    }

    void vDHTSensorTask(void* parameter)
    {
        Types::DisplayData displayData;

        Types::DHTSensorData dhtData;
        dhtData.temperature = 0.0f;
        dhtData.humidity = 0.0f;

        QueueHandle_t displayQueue = Queues::displayQueue;
        QueueHandle_t dhtSensorQueue = Queues::dhtSensorQueue;

        DHT dhtSensor(PIN_IN_DIG, DHT22);
        dhtSensor.begin();

        // A little delay for the sensor to stabilize
        vTaskDelay(pdMS_TO_TICKS(2000));

        for (;;) 
        {
            float h = dhtSensor.readHumidity();
            float t = dhtSensor.readTemperature();

            // 4. Verificação de erros (isnan = is not a number)
            if (isnan(h) || isnan(t)) 
            {
                Serial.println("Failed to read from DHT!");
            } 
            else 
            {
                dhtData.humidity = h;
                dhtData.temperature = t;

                displayData.value = static_cast<void*>(&dhtData);

                xQueueSendToBack(displayQueue, &displayData, 0);
                xQueueOverwrite(dhtSensorQueue, &dhtData);
            }

            vTaskDelay(pdMS_TO_TICKS(DHT_TASK_DELAY_MS));
        }
    }

    void vPIRSensorTask(void* parameter)
    {
        return;
    }

    void vLDRSensorTask(void* parameter)
    {
        return;
    }

    void vBuzzerTask(void* parameter)
    {
        for (int i = 0; i < 5; i++) 
        {
            ledcWrite(0, 250); // 50% duty cycle
            vTaskDelay(pdMS_TO_TICKS(100));
            ledcWrite(0, 0);   // Turn off
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        Serial.println("Buzzer finalizado!");
        vTaskDelete(NULL);
    }

    bool vSysMonitorIdleHook()
    {
        static TickType_t lastTick = 0;
        static uint32_t localIdleCounter = 0;
        static TickType_t measurementStartTick = 0; 
        
        if (measurementStartTick == 0) 
        {
            measurementStartTick = xTaskGetTickCount();
        }

        TickType_t currentTick = xTaskGetTickCount();

        if (currentTick != lastTick) 
        {
            localIdleCounter++;
            lastTick = currentTick;
        }

        TickType_t totalTimePassed = currentTick - measurementStartTick;

        if (totalTimePassed >= 1000) 
        {
            
            Types::SystemMetrics msg;
            msg.idleTicks = localIdleCounter;
            msg.totalTicks = totalTimePassed; // Isso será ~1000 (ms)

            // Envia para a Queue
            xQueueSend(Queues::sysMonitorQueue, &msg, 0);

            // Reseta para a próxima janela
            localIdleCounter = 0;
            measurementStartTick = currentTick; // Novo marco zero
        }

        return true;
    }
}
