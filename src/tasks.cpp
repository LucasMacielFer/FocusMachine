#include "tasks.h"

/* Ultimate stack monitoring device 2000 ultra-XPTO
    UBaseType_t stackSobra = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("[DisplayTask] Stack Livre: %u bytes\n", (unsigned int)stackSobra);
*/

namespace Tasks
{
    void vTasksInitialize()
    {
        TaskHandle_t handlePomodoroFSMTask;
        TaskHandle_t handleDisplayTask;
        TaskHandle_t handleCameraInferenceTask;
        TaskHandle_t handleDHTSensorTask;
        TaskHandle_t handlePIRSensorTask;
        TaskHandle_t handleLDRSensorTask;

        xTaskCreatePinnedToCore(vPomodoroFSMTask, "PomodoroFSMTask", 8192, NULL, 2, &handlePomodoroFSMTask, 0);
        xTaskCreatePinnedToCore(vDisplayTask, "DisplayTask", 8192, NULL, 2, &handleDisplayTask, 0);
        xTaskCreatePinnedToCore(vCameraInferenceTask, "CameraInferenceTask", 8192, NULL, 1, &handleCameraInferenceTask, 1);
        xTaskCreatePinnedToCore(vDHTSensorTask, "DHTSensorTask", 4096, NULL, 1, &handleDHTSensorTask, 0);
        xTaskCreatePinnedToCore(vPIRSensorTask, "PIRSensorTask", 4096, NULL, 1, &handlePIRSensorTask, 0);
        xTaskCreatePinnedToCore(vLDRSensorTask, "LDRSensorTask", 4096, NULL, 1, &handleLDRSensorTask, 0);

        TaskHandle_t* handlers[6] = {&handlePomodoroFSMTask, &handleDisplayTask, &handleCameraInferenceTask, &handleDHTSensorTask, &handlePIRSensorTask, &handleLDRSensorTask};
        xTaskCreatePinnedToCore(vBrainTask, "BrainTask", 16384, (void*) handlers, 3, NULL, 0);

        esp_register_freertos_idle_hook(Tasks::vSysMonitorIdleHook);
    }

    void vPomodoroFSMTask(void* parameter)
    {
        // 30 seconds just for testing purposes
        //TODO: Write this all properly
        int minutes = 0;
        int seconds = 30;
        int prevMinutes = minutes;
        int prevSeconds = seconds;
        int digit0 = seconds % 10;
        int digit1 = seconds / 10;
        int digit2 = minutes % 10;
        int digit3 = minutes / 10;
        bool changeVector[4] = {true, true, true, true}; // To track which digits need updating
        bool startTick = true;

        Types::PomodoroState currentState = Types::PomodoroState::FOCUS;
        Types::SystemState currentSysState = Types::SystemState::TIMER;

        Types::DisplayData bundle;
        Types::DisplayData bundle_0;
        Types::DisplayData bundle_1;
        Types::DisplayData bundle_2;
        Types::DisplayData bundle_3;

        bundle_0.type = Types::DataType::TIME_0;
        bundle_1.type = Types::DataType::TIME_1;
        bundle_2.type = Types::DataType::TIME_2;
        bundle_3.type = Types::DataType::TIME_3;

        xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, 0);
        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
        {
            if(currentSysState == Types::SystemState::TIMER)
            {
                if(startTick)
                {
                    Serial.println("\n\n\nStarting timer tick...\n\n\n");
                    bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                    bundle.value = static_cast<void*>(
                        new Types::ScreenChangeRequest{
                            currentState,
                            currentSysState,
                            {minutes, seconds}
                        }
                    );
                    xQueueSendToBack(Queues::displayQueue, &bundle, portMAX_DELAY);
                    bundle_0.value = static_cast<void*>(&digit0);
                    xQueueSendToBack(Queues::displayQueue, &bundle_0, 0);
                    bundle_1.value = static_cast<void*>(&digit1);
                    xQueueSendToBack(Queues::displayQueue, &bundle_1, 0);
                    bundle_2.value = static_cast<void*>(&digit2);
                    xQueueSendToBack(Queues::displayQueue, &bundle_2, 0);
                    bundle_3.value = static_cast<void*>(&digit3);
                    xQueueSendToBack(Queues::displayQueue, &bundle_3, 0);
                    xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
                    startTick = false;
                }

                if(seconds > 0) 
                {
                    seconds--;
                    changeVector[0] = true;
                    changeVector[1] = prevSeconds/10 != seconds/10;
                    changeVector[2] = false;
                    changeVector[3] = false;
                }
                else
                {
                    if(minutes > 1)
                    {
                        minutes--;
                        seconds = 59;
                        changeVector[0] = true;
                        changeVector[1] = true;
                        changeVector[2] = true;
                        changeVector[3] = prevMinutes/10 != minutes/10;
                    }
                    else
                    {
                        minutes = 0;
                        seconds = 0;
                        changeVector[0] = true;
                        changeVector[1] = true;
                        changeVector[2] = true;
                        changeVector[3] = true;
                    }
                }

                prevMinutes = minutes;
                prevSeconds = seconds;

                if(changeVector[0])
                {
                    digit0 = seconds % 10;
                    bundle_0.value = static_cast<void*>(&digit0);
                    xQueueSendToBack(Queues::displayQueue, &bundle_0, 0);
                }
                if(changeVector[1])
                {
                    digit1 = seconds / 10;
                    bundle_1.value = static_cast<void*>(&digit1);
                    xQueueSendToBack(Queues::displayQueue, &bundle_1, 0);
                }
                if(changeVector[2])
                {
                    digit2 = minutes % 10;
                    bundle_2.value = static_cast<void*>(&digit2);
                    xQueueSendToBack(Queues::displayQueue, &bundle_2, 0);
                }
                if(changeVector[3])
                {
                    digit3 = minutes / 10;
                    bundle_3.value = static_cast<void*>(&digit3);
                    xQueueSendToBack(Queues::displayQueue, &bundle_3, 0);
                }
                
                if (minutes <= 0 && seconds <= 0) 
                {
                    currentSysState = Types::SystemState::FINISHED;
                }
                else
                    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            }

            else if(currentSysState == Types::SystemState::FINISHED)
            {
                bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                    bundle.value = static_cast<void*>(
                        new Types::ScreenChangeRequest{
                            currentState,
                            Types::SystemState::FINISHED,
                            {0, 0}
                        }
                    );

                xQueueSendToBack(Queues::displayQueue, &bundle, 0);
                xQueueSendToBack(Queues::SystemStateQueue, &currentSysState, 0);
                xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
                //xSemaphoreTake(Semaphores::buttonSemaphore, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(5000));
                
                switch(currentState)
                {
                    case Types::PomodoroState::FOCUS:
                        currentState = Types::PomodoroState::SHORT_BREAK;
                        minutes = 0; // For testing purposes
                        seconds = 10;
                        break;
                    case Types::PomodoroState::SHORT_BREAK:
                        currentState = Types::PomodoroState::FOCUS;
                        minutes = 0; // For testing purposes
                        seconds = 10;
                        break;
                    default:
                        break;
                }

                currentSysState = Types::SystemState::TIMER;
                Serial.println("\n\n\n\n\nStarting new session...\n\n\n\n");
                startTick = true;
            }
        }
    }
    
    void vBrainTask(void* parameter)
    {
        TaskHandle_t handlePomodoroFSMTask;
        TaskHandle_t handleDisplayTask;
        TaskHandle_t handleCameraInferenceTask;
        TaskHandle_t handleDHTSensorTask;
        TaskHandle_t handlePIRSensorTask;
        TaskHandle_t handleLDRSensorTask;

        TaskHandle_t** handlers = (TaskHandle_t**) parameter;
        handlePomodoroFSMTask = *(handlers[0]);
        handleDisplayTask = *(handlers[1]);
        handleCameraInferenceTask = *(handlers[2]);
        handleDHTSensorTask = *(handlers[3]);
        handlePIRSensorTask = *(handlers[4]);
        handleLDRSensorTask = *(handlers[5]);
        
        Types::DHTSensorData dhtData;
        Types::SystemMetrics sysMetrics;
        Types::StateChangeRequest currentSystemState;

        Algorithms::ComfortConfig comfortConfig = {
            .idealTemp = 23.0f, .tempTol = 7.0f,
            .idealHum = 50.0f,  .humTol = 25.0f,
            .luxIdealMin = 1800, .luxIdealMax = 3200, .luxTol = 1000,
            .pirMaxEvents = 4
        };

        float cpuUsage;
        int PIRdetections;
        int luminosity;
        bool faceDetected;

        int motionEvents = 0;
        int cameraEvents = 0;
        int cameraDetections = 0;
        float luminosityPercentage = 0.0f;

        bool detectionDebounce = false;
        int debounceCounter = 0;

        float comfortIndex;
        float focusIndex;

        bool recalculateComfortIndex = false;
        bool recalculateFocusIndex = false;

        QueueHandle_t dhtSensorQueue = Queues::dhtSensorQueue;
        QueueHandle_t sysMonitorQueue = Queues::sysMonitorQueue;
        QueueHandle_t pirSensorQueue = Queues::pirSensorQueue;
        QueueHandle_t ldrSensorQueue = Queues::ldrSensorQueue;
        QueueHandle_t displayQueue = Queues::displayQueue;
        QueueHandle_t cameraInferenceQueue = Queues::cameraInferenceQueue;
        QueueHandle_t SystemStateQueue = Queues::SystemStateQueue;

        // Queue set
        QueueSetHandle_t brainQueueSet = xQueueCreateSet(6); 

        xQueueAddToSet(dhtSensorQueue, brainQueueSet);
        xQueueAddToSet(sysMonitorQueue, brainQueueSet);
        xQueueAddToSet(pirSensorQueue, brainQueueSet);
        xQueueAddToSet(ldrSensorQueue, brainQueueSet);
        xQueueAddToSet(cameraInferenceQueue, brainQueueSet);
        xQueueAddToSet(SystemStateQueue, brainQueueSet);

        QueueSetMemberHandle_t xActivatedMember;

        for(;;)
        {
            xActivatedMember = xQueueSelectFromSet(brainQueueSet, portMAX_DELAY);
            if(xActivatedMember == dhtSensorQueue) 
            {
                Types::DisplayData displayDataTemp;
                Types::DisplayData displayDataHum;

                if(xQueueReceive(dhtSensorQueue, &dhtData, 0) == pdTRUE)
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
                    recalculateComfortIndex = true;
                }
            }

            if(xActivatedMember == pirSensorQueue) 
            {
                if(xQueueReceive(pirSensorQueue, &PIRdetections, 0) == pdTRUE)
                {
                    motionEvents += PIRdetections;
                    Serial.print("PIR Detections: ");
                    Serial.println(PIRdetections);
                    recalculateComfortIndex = true;
                }
            }

            if(xActivatedMember == ldrSensorQueue) 
            {
                if(xQueueReceive(ldrSensorQueue, &luminosity, 0) == pdTRUE)
                {
                    Types::DisplayData displayDataLdr;
                    displayDataLdr.type = Types::DataType::LUMINOSITY;

                    luminosityPercentage = (static_cast<float>(luminosity) / 4095.0f) * 100.0f;

                    displayDataLdr.value = static_cast<void*>(&luminosityPercentage);
                    xQueueSendToBack(displayQueue, &displayDataLdr, 0);
                    Serial.print("Luminosity: ");
                    Serial.println(luminosity);
                    recalculateComfortIndex = true;
                }
            }

            if(xActivatedMember == cameraInferenceQueue) 
            {
                if(xQueueReceive(cameraInferenceQueue, &faceDetected, 0) == pdTRUE)
                {
                    Serial.println("Camera Inference Data Received");

                    if(faceDetected) 
                        detectionDebounce = true;

                    debounceCounter++;

                    if(debounceCounter >= CAMERA_DEBOUNCE_THRESHOLD) 
                    {
                        if(detectionDebounce) 
                        {
                            cameraDetections++;
                            Serial.println("Face detection counted!");
                        }
                        recalculateFocusIndex = true;
                        detectionDebounce = false;
                        debounceCounter = 0;
                        cameraEvents++;
                    }
                }
            }

            if(xActivatedMember == SystemStateQueue) 
            {
                if(xQueueReceive(SystemStateQueue, &currentSystemState, 0) == pdTRUE)
                {
                    switch(currentSystemState.sysState)
                    {
                        case Types::SystemState::TIMER:
                            Serial.println("System State changed to TIMER");
                            if(currentSystemState.pomodoroState == Types::PomodoroState::FOCUS) 
                            {
                                ISR::attachPIRISR();
                                vTaskResume(handleCameraInferenceTask);
                                vTaskResume(handleDHTSensorTask);
                                vTaskResume(handlePIRSensorTask);
                                vTaskResume(handleLDRSensorTask);
                            }
                            break;
                        case Types::SystemState::ADJUST:
                            Serial.println("System State changed to ADJUST");
                            vTaskResume(handleCameraInferenceTask);
                            vTaskSuspend(handleDHTSensorTask);
                            vTaskSuspend(handlePIRSensorTask);
                            vTaskSuspend(handleLDRSensorTask);
                            ISR::detachPIRISR();
                            break;
                        case Types::SystemState::FINISHED:
                            Serial.println("System State changed to FINISHED");
                            xTaskCreatePinnedToCore(vBuzzerTask, "Task buzzer", 1024, NULL, 1, NULL, 1);
                            vTaskSuspend(handleCameraInferenceTask);
                            vTaskSuspend(handleDHTSensorTask);
                            vTaskSuspend(handlePIRSensorTask);
                            vTaskSuspend(handleLDRSensorTask);
                            ISR::detachPIRISR();
                            focusIndex = 0.0f;
                            motionEvents = 0;
                            cameraEvents = 0;
                            cameraDetections = 0;
                            break;
                        case Types::SystemState::PAUSED:
                            Serial.println("System State changed to PAUSED");
                            vTaskSuspend(handleCameraInferenceTask);
                            vTaskSuspend(handleDHTSensorTask);
                            vTaskSuspend(handlePIRSensorTask);
                            vTaskSuspend(handleLDRSensorTask);
                            ISR::detachPIRISR();
                            break;
                        default:
                            break;
                    }
                }
            }

            // Read system metrics
            if(xActivatedMember == sysMonitorQueue) 
            {
                if(xQueueReceive(sysMonitorQueue, &sysMetrics, 0) == pdTRUE)
                {
                    if(sysMetrics.totalTicks > 0)
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
            }

            if(recalculateComfortIndex)
            {
                comfortIndex = Algorithms::calculateComfortIndex(dhtData.temperature, dhtData.humidity, luminosity, motionEvents, comfortConfig);
                Types::DisplayData displayDataComfort;
                displayDataComfort.type = Types::DataType::COMFORT_INDEX;
                displayDataComfort.value = static_cast<void*>(&comfortIndex);
                xQueueSendToBack(displayQueue, &displayDataComfort, 0);
                recalculateComfortIndex = false;
            }

            if(recalculateFocusIndex)
            {
                focusIndex = Algorithms::calculateFocusIndex(cameraDetections, cameraEvents);
                Types::DisplayData displayDataFocus;
                displayDataFocus.type = Types::DataType::FOCUS_INDEX;
                displayDataFocus.value = static_cast<void*>(&focusIndex);
                xQueueSendToBack(displayQueue, &displayDataFocus, 0);
                recalculateFocusIndex = false;
            }
        }
    }

    void vDisplayTask(void* parameter)
    {
        QueueHandle_t myQueue = Queues::displayQueue;
        Types::DisplayData displayData;

        float* floatBuffer = nullptr;
        int* intBuffer = nullptr;

        Adafruit_ILI9341* tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

        tft->begin();
        tft->setRotation(1);
        xSemaphoreGive(Semaphores::displayPomodoroHandshakeSemaphore);
        for(;;)
        {
            if(xQueueReceive(myQueue, &displayData, portMAX_DELAY) == pdTRUE)
            {
                switch(displayData.type)
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
                    case Types::DataType::TIME_0:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTime(tft, Types::DataType::TIME_0, *intBuffer);
                        break;
                    }
                    case Types::DataType::TIME_1:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTime(tft, Types::DataType::TIME_1, *intBuffer);
                        break;
                    }
                    case Types::DataType::TIME_2:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTime(tft, Types::DataType::TIME_2, *intBuffer);
                        break;
                    }
                    case Types::DataType::TIME_3:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTime(tft, Types::DataType::TIME_3, *intBuffer);
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
                    case Types::DataType::ADJ_TIME_0:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTimerAdjustment(tft, Types::DataType::ADJ_TIME_0, *intBuffer);
                        break;
                    }
                    case Types::DataType::ADJ_TIME_1:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTimerAdjustment(tft, Types::DataType::ADJ_TIME_1, *intBuffer);
                        break;
                    }
                    case Types::DataType::ADJ_TIME_2:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTimerAdjustment(tft, Types::DataType::ADJ_TIME_2, *intBuffer);
                        break;
                    }
                    case Types::DataType::ADJ_TIME_3:
                    {
                        intBuffer = (int*) (displayData.value);
                        Display::vPrintTimerAdjustment(tft, Types::DataType::ADJ_TIME_3, *intBuffer);
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
                        xSemaphoreGive(Semaphores::displayPomodoroHandshakeSemaphore);
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

        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
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

            xQueueOverwrite(Queues::cameraInferenceQueue, &detected);
            Serial.println("Camera inference data sent!");

            esp_camera_fb_return(fb);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAMERA_TASK_DELAY_MS));
        }
    }

    void vDHTSensorTask(void* parameter)
    {
        Types::DHTSensorData dhtData;
        dhtData.temperature = 0.0f;
        dhtData.humidity = 0.0f;

        QueueHandle_t displayQueue = Queues::displayQueue;
        QueueHandle_t dhtSensorQueue = Queues::dhtSensorQueue;

        DHT dhtSensor(PIN_IN_DIG, DHT22);
        dhtSensor.begin();

        // A little delay for the sensor to stabilize
        vTaskDelay(pdMS_TO_TICKS(2000));

        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;) 
        {
            float h = dhtSensor.readHumidity();
            float t = dhtSensor.readTemperature();

            // 4. Verificação de erros (isnan = is not a number)
            if(isnan(h) || isnan(t)) 
            {
                Serial.println("Failed to read from DHT!");
            } 
            else 
            {
                dhtData.humidity = h;
                dhtData.temperature = t;

                xQueueOverwrite(dhtSensorQueue, &dhtData);
                Serial.println("DHT data sent!");
            }

            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DHT_TASK_DELAY_MS));
        }
    }

    void vPIRSensorTask(void* parameter)
    {
        int pirValue = 0;
        int result = 0;
        QueueHandle_t pirSensorQueue = Queues::pirSensorQueue;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
        {
            while(xSemaphoreTake(Semaphores::pirEventSemaphore, 0) == pdTRUE)
            {
                pirValue++;
                Serial.println("PIR Motion Detected!");
            }

            result = pirValue;
            xQueueOverwrite(pirSensorQueue, &result);
            Serial.println("PIR value sent!");
            pirValue = 0;

            vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(PIR_TASK_DELAY_MS));
        }
    }

    void vLDRSensorTask(void* parameter)
    {
        int ldrValue = 0;
        QueueHandle_t ldrSensorQueue = Queues::ldrSensorQueue;
        TickType_t xLastWakeTime = xTaskGetTickCount();
        for(;;)
        {
            ldrValue = analogRead(PIN_IN_ANLG);
            xQueueOverwrite(ldrSensorQueue, &ldrValue);
            Serial.println("LDR value sent!");
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LDR_TASK_DELAY_MS));
        }
    }

    void vBuzzerTask(void* parameter)
    {
        for(int i = 0; i < 5; i++) 
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
        
        if(measurementStartTick == 0) 
        {
            measurementStartTick = xTaskGetTickCount();
        }

        TickType_t currentTick = xTaskGetTickCount();

        if(currentTick != lastTick) 
        {
            localIdleCounter++;
            lastTick = currentTick;
        }

        TickType_t totalTimePassed = currentTick - measurementStartTick;

        if(totalTimePassed >= 1000) 
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