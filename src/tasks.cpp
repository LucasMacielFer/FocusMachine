#include "tasks.h"

/* Ultimate stack monitoring device 2000 ultra-XPTO
    UBaseType_t stackSobra = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("[DisplayTask] Stack Livre: %u bytes\n", (unsigned int)stackSobra);
*/

namespace Tasks
{
    void vTasksInitialize()
    {
        xTaskCreatePinnedToCore(vPomodoroFSMTask, "PomodoroFSM", 4096, NULL, 4, NULL, 0);
        xTaskCreatePinnedToCore(vBrainTask, "BrainTask", 8192, NULL, 3, NULL, 0);
        xTaskCreatePinnedToCore(vDisplayTask, "DisplayTask", 16384, NULL, 2, NULL, 0);
        xTaskCreatePinnedToCore(vCameraInferenceTask, "CameraInferenceTask", 8192, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(vDHTSensorTask, "DHTSensorTask", 4096, NULL, 1, NULL, 0);
        //xTaskCreatePinnedToCore(vPIRSensorTask, "PIRSensorTask", 4096, NULL, 1, NULL, 0);
        //xTaskCreatePinnedToCore(vLDRSensorTask, "LDRSensorTask", 4096, NULL, 1, NULL, 0);

        esp_register_freertos_idle_hook(Tasks::vSysMonitorIdleHook);
    }

    void vPomodoroFSMTask(void* parameter)
    {
        // 30 seconds just for testing purposes
        //TODO: Write this all properly
        int minutes = 0;
        int seconds = 30;
        Types::StopWatchTime time;
        Types::DisplayData bundle;
        bundle.type = Types::DataType::TIME;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
        {
            if (seconds > 0) 
                seconds--;
            else
            {
                if(minutes > 1)
                {
                    minutes--;
                    seconds = 59;
                }
                else
                {
                    minutes = 0;
                    seconds = 0;
                }
            }
            
            time.minutes = minutes;
            time.seconds = seconds;
            bundle.value = static_cast<void*>(&time);   

            xQueueSendToBack(Queues::displayQueue, &bundle, 0);
            
            if (minutes <= 0 && seconds <= 0) 
            {
                xTaskCreatePinnedToCore(vBuzzerTask, "Task buzzer", 1024, NULL, 1, NULL, 1);
                bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                bundle.value = static_cast<void*>(
                    new Types::ScreenChangeRequest{
                        Types::PomodoroState::FOCUS,
                        Types::SystemState::FINISHED,
                        {0, 0}
                    }
                );
                xQueueSendToBack(Queues::displayQueue, &bundle, 0);
                vTaskDelete(NULL);
            }

            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
        }
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

        // TEMPORARY!!!!!!
        //TODO Remove this
        Types::DisplayData displayDataRequest;
        displayDataRequest.type = Types::DataType::SCREEN_CHANGE_REQUEST;
        displayDataRequest.value = static_cast<void*>(
            new Types::ScreenChangeRequest{
                Types::PomodoroState::FOCUS,
                Types::SystemState::TIMER,
                {0, 0}
            }
        );

        xQueueSendToBack(displayQueue, &displayDataRequest, 0);
        for(;;)
        {
            xActivatedMember = xQueueSelectFromSet(brainQueueSet, portMAX_DELAY);
            if (xActivatedMember == dhtSensorQueue) 
            {
                Types::DisplayData displayDataTemp;
                Types::DisplayData displayDataHum;

                if (xQueueReceive(dhtSensorQueue, &dhtData, 0) == pdTRUE)
                {
                    displayDataTemp.type = Types::DataType::TEMPERATURE;
                    displayDataTemp.value = static_cast<void*>(&dhtData.temperature);
                    displayDataHum.type = Types::DataType::HUMIDITY;
                    displayDataHum.value = static_cast<void*>(&dhtData.humidity);

                    Serial.print("Temperature: ");
                    Serial.print(dhtData.temperature);
                    Serial.print(" °C, Humidity: ");
                    Serial.print(dhtData.humidity);
                    Serial.println(" %");

                    xQueueSendToBack(displayQueue, &displayDataTemp, 0);
                    xQueueSendToBack(displayQueue, &displayDataHum, 0);
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
                        Serial.println("Error: TotalTicks is 0");
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
                //TODO : Process camera inference data
                if (xQueueReceive(cameraInferenceQueue, &faceDetected, 0) == pdTRUE)
                {
                    Serial.println("Camera Inference Data Received");
                }
            }
        }
    }

    void vDisplayTask(void* parameter)
    {
        QueueHandle_t myQueue = Queues::displayQueue;
        Types::DisplayData displayData;

        float* floatBuffer;
        int* intBuffer = (int*) malloc(sizeof(int) * 2);

        Adafruit_ILI9341* tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

        tft->begin();
        tft->setRotation(1);
        
        for(;;)
        {
            if (xQueueReceive(myQueue, &displayData, portMAX_DELAY) == pdTRUE)
            {
                //TODO - remove
                switch (displayData.type)
                {
                    case Types::DataType::TEMPERATURE:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintTemperature(tft, *floatBuffer);
                        break;
                    }
                    case Types::DataType::HUMIDITY:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintHumidity(tft, *floatBuffer);
                        break;
                    }
                    case Types::DataType::LUMINOSITY:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintLuminosity(tft, *floatBuffer);
                        break;
                    }
                    case Types::DataType::TIME:
                    {
                        intBuffer = *((int (*)[2]) (displayData.value));
                        Display::vPrintTime(tft, intBuffer[0], intBuffer[1]);
                        break;
                    }
                    case Types::DataType::FOCUS_INDEX:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintFocusIndex(tft, *floatBuffer);
                        break;
                    }
                    case Types::DataType::COMFORT_INDEX:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintComfortIndex(tft, *floatBuffer);
                        break;
                    }
                    case Types::DataType::ADJ_TIME:
                    {
                        intBuffer = *((int (*)[2]) (displayData.value));
                        Display::vPrintTimerAdjustment(tft, intBuffer[0], intBuffer[1]);
                        break;
                    }
                    case Types::DataType::SCREEN_CHANGE_REQUEST:
                    {
                        Types::ScreenChangeRequest screenChange = *(Types::ScreenChangeRequest*)displayData.value;
                        Types::PomodoroState newPomodoroState = screenChange.pomodoroState;
                        Types::SystemState newSystemState = screenChange.systemState;
                        switch (newSystemState)
                        {
                            case Types::SystemState::TIMER:
                                Display::vInitializeTimerDisplay(tft, newPomodoroState);
                                break;
                            case Types::SystemState::ADJUST:
                                Display::vPrintAdjustingTimer(tft, newPomodoroState, screenChange.timerCount[0], screenChange.timerCount[1]);
                                break;
                            case Types::SystemState::FINISHED:
                                Display::vPrintStateFinished(tft, newPomodoroState);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
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
                Serial.printf("FACE DETECTED! Accuracy: %.2f%%\n", acc * 100);
            }
            else 
            {
                Serial.println("No face detected...");
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

                xQueueOverwrite(dhtSensorQueue, &dhtData);
            }

            vTaskDelay(pdMS_TO_TICKS(DHT_TASK_DELAY_MS));
        }
    }

    void vPIRSensorTask(void* parameter)
    {
        //TODO: Implement PIR Sensor Task
        return;
    }

    void vLDRSensorTask(void* parameter)
    {
        //TODO: Implement LDR Sensor Task
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
        Serial.println("Buzzer finished!");
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
