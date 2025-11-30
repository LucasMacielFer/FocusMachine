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

        xTaskCreatePinnedToCore(vPomodoroFSMTask, "PomodoroFSMTask", 8192, NULL, 4, &handlePomodoroFSMTask, 0);
        xTaskCreatePinnedToCore(vDisplayTask, "DisplayTask", 8192, NULL, 2, &handleDisplayTask, 0);
        xTaskCreatePinnedToCore(vCameraInferenceTask, "CameraInferenceTask", 16384, NULL, 1, &handleCameraInferenceTask, 1);
        xTaskCreatePinnedToCore(vDHTSensorTask, "DHTSensorTask", 4096, NULL, 1, &handleDHTSensorTask, 0);
        xTaskCreatePinnedToCore(vPIRSensorTask, "PIRSensorTask", 4096, NULL, 1, &handlePIRSensorTask, 0);
        xTaskCreatePinnedToCore(vLDRSensorTask, "LDRSensorTask", 4096, NULL, 1, &handleLDRSensorTask, 0);

        TaskHandle_t* handlers[6] = {&handlePomodoroFSMTask, &handleDisplayTask, &handleCameraInferenceTask, &handleDHTSensorTask, &handlePIRSensorTask, &handleLDRSensorTask};
        xTaskCreatePinnedToCore(vBrainTask, "BrainTask", 16384, (void*) handlers, 3, NULL, 0);

        esp_register_freertos_idle_hook(Tasks::vSysMonitorIdleHook);
    }

    void vPomodoroFSMTask(void* parameter)
    {
        // Default durations
        //TODO: Invert durations - Those are for testing
        int timerDurationFocus[2] = {0, 25};       // 25 minutes
        int timerDurationShortBreak[2] = {0, 10};   // 5 minutes
        int timerDurationLongBreak[2] = {0, 15};   // 15 minutes

        int shortBreakCounter = 0;
        Types::StopWatchTime currentTime = { -1, -1 };

        int minutes = 0;
        int seconds = 30;
        bool startTick = true;

        int adjustDigitsIndex = 3;
        int adjustBuffer[4] = {0, 0, 0, 0};
        bool highlightAdjust[4] = {false, false, false, false};

        const TickType_t xDelay = pdMS_TO_TICKS(1000);
        TickType_t xNextWakeTime = xTaskGetTickCount() + xDelay;

        int receivedEvent = -1;
        uint32_t lastTouch1ProcessedTime = 0;
        uint32_t lastTouch2ProcessedTime = 0;

        Types::PomodoroState currentPomodoroState = Types::PomodoroState::FOCUS;
        Types::PomodoroState nextPomodoroState = currentPomodoroState;
        Types::SystemState currentSysState = Types::SystemState::ADJUST;
        Types::SystemState nextSysState = currentSysState;

        Types::DisplayData bundle;
        Types::DisplayData timeBundle;

        timeBundle.type = Types::DataType::UPDATE_TIME;

        xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
        Types::StateChangeRequest initialState {
            currentSysState,
            currentPomodoroState
        };
        xQueueSendToBack(Queues::systemStateQueue, &initialState, portMAX_DELAY);
        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
        {
            nextSysState = currentSysState;
            nextPomodoroState = currentPomodoroState;

            if(currentSysState == Types::SystemState::TIMER)
            {
                if(startTick)
                {
                    Serial.println("\n\n\nStarting timer tick...\n\n\n");
                    bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                    Types::ScreenChangeRequest request {
                            currentPomodoroState,
                            currentSysState,
                            {minutes, seconds}};

                    bundle.value = static_cast<void*>(&request);
                    xQueueSendToBack(Queues::displayQueue, &bundle, portMAX_DELAY);
                    currentTime.seconds = seconds;
                    currentTime.minutes = minutes;
                    timeBundle.type = Types::DataType::UPDATE_TIME;
                    timeBundle.value = static_cast<void*>(&currentTime);
                    xQueueSendToBack(Queues::displayQueue, &timeBundle, 0);
                    xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
                    startTick = false;
                }

                TickType_t xNow = xTaskGetTickCount();
                TickType_t xTicksToWait = 0;
                if (xNextWakeTime > xNow) 
                    xTicksToWait = xNextWakeTime - xNow;
                
                if (xQueueReceive(Queues::interactionEventQueue, &receivedEvent, xTicksToWait) == pdTRUE)
                {
                    if (receivedEvent == FROM_BUTTON) 
                    {
                        Serial.println("\n\n\n\nTimer paused by user.\n\n\n\n");
                        nextSysState = Types::SystemState::PAUSED;
                    }
                }

                else
                {
                    
                    if (seconds > 0) 
                    {
                        seconds--;
                    } 
                    else 
                    {
                        if (minutes > 0) 
                        {
                            minutes--;
                            seconds = 59;
                        } 
                        else 
                        {
                            nextSysState = Types::SystemState::FINISHED;
                        }
                    }

                    currentTime.minutes = minutes;
                    currentTime.seconds = seconds;
                    timeBundle.type = Types::DataType::UPDATE_TIME;
                    timeBundle.value = static_cast<void*>(&currentTime);
                    xQueueSendToBack(Queues::displayQueue, &timeBundle, 0);

                    xNextWakeTime += xDelay;
                }
                
                if (minutes <= 0 && seconds <= 0) 
                {
                    nextSysState = Types::SystemState::FINISHED;
                }
            }

            else if(currentSysState == Types::SystemState::FINISHED)
            {
                bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                Types::ScreenChangeRequest request {
                        currentPomodoroState,
                        currentSysState,
                        {minutes, seconds}};
                bundle.value = static_cast<void*>(&request);

                xQueueSendToBack(Queues::displayQueue, &bundle, 0);
                xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);

                // Awaits user interaction to start next state
                xQueueReceive(Queues::interactionEventQueue, &receivedEvent, portMAX_DELAY);
                Serial.println("User interaction received, starting next state...");
                Serial.println(receivedEvent);
                
                switch(currentPomodoroState)
                {
                    case Types::PomodoroState::FOCUS:
                        if(shortBreakCounter < 3)
                        {
                            nextPomodoroState = Types::PomodoroState::SHORT_BREAK;
                            minutes = timerDurationShortBreak[0]; 
                            seconds = timerDurationShortBreak[1];
                            shortBreakCounter++;
                        }
                        else
                        {
                            nextPomodoroState = Types::PomodoroState::LONG_BREAK;
                            minutes = timerDurationLongBreak[0]; 
                            seconds = timerDurationLongBreak[1];
                            shortBreakCounter = 0;
                        }
                        break;
                    case Types::PomodoroState::SHORT_BREAK:
                    case Types::PomodoroState::LONG_BREAK:
                        nextPomodoroState = Types::PomodoroState::FOCUS;
                        minutes = timerDurationFocus[0]; 
                        seconds = timerDurationFocus[1];
                        break;
                    default:
                        break;
                }

                nextSysState = Types::SystemState::TIMER;
                Serial.println("\n\n\n\n\nStarting new session...\n\n\n\n");
                startTick = true;
            }

            else if (currentSysState == Types::SystemState::ADJUST)
            {
                if(startTick)
                {
                    switch(currentPomodoroState)
                    {
                        case Types::PomodoroState::FOCUS:
                            minutes = timerDurationFocus[0];
                            seconds = timerDurationFocus[1];
                            break;
                        case Types::PomodoroState::SHORT_BREAK:
                            minutes = timerDurationShortBreak[0];
                            seconds = timerDurationShortBreak[1];
                            break;
                        case Types::PomodoroState::LONG_BREAK:
                            minutes = timerDurationLongBreak[0];
                            seconds = timerDurationLongBreak[1];
                            break;
                    }
                    adjustDigitsIndex = 3;
                    highlightAdjust[0] = false;
                    highlightAdjust[1] = false;
                    highlightAdjust[2] = false;
                    highlightAdjust[3] = true;

                    bundle.type = Types::DataType::SCREEN_CHANGE_REQUEST;
                    Types::ScreenChangeRequest request{
                            currentPomodoroState,
                            currentSysState,
                            {minutes, seconds}
                        };
                    bundle.value = static_cast<void*>(&request);

                    adjustBuffer[0] = seconds % 10;
                    adjustBuffer[1] = seconds / 10;
                    adjustBuffer[2] = minutes % 10;
                    adjustBuffer[3] = minutes / 10;
                    xQueueSendToBack(Queues::displayQueue, &bundle, portMAX_DELAY);
                    xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
                    startTick = false;
                }

                xQueueReceive(Queues::interactionEventQueue, &receivedEvent, portMAX_DELAY);
                if(receivedEvent == FROM_TOUCH1)
                {
                    uint32_t now = millis();
                    if(now - lastTouch1ProcessedTime > DEBOUNCE_INTERVAL_MS)
                    {
                        lastTouch1ProcessedTime = now;
                        if(adjustDigitsIndex > 0)
                        {
                            highlightAdjust[adjustDigitsIndex] = false;
                            adjustDigitsIndex--;
                            highlightAdjust[adjustDigitsIndex] = true;
                        }
                        else
                        {
                            highlightAdjust[0] = false;
                            highlightAdjust[3] = true;
                            adjustDigitsIndex = 3;

                            switch(currentPomodoroState)
                            {
                                case Types::PomodoroState::FOCUS:
                                    timerDurationFocus[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                                    timerDurationFocus[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                                    nextPomodoroState = Types::PomodoroState::SHORT_BREAK;
                                    break;
                                case Types::PomodoroState::SHORT_BREAK:
                                    timerDurationShortBreak[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                                    timerDurationShortBreak[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                                    nextPomodoroState = Types::PomodoroState::LONG_BREAK;
                                    break;
                                case Types::PomodoroState::LONG_BREAK:
                                    timerDurationLongBreak[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                                    timerDurationLongBreak[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                                    nextPomodoroState = Types::PomodoroState::FOCUS;
                                    break;
                            }
                            startTick = true;
                        }

                        timeBundle.type = Types::DataType::ADJUST_TIME;
                        Types::StopWatchTimeAdjustment timeAdjust;
                        timeAdjust.minutes = adjustBuffer[3]*10 + adjustBuffer[2];
                        timeAdjust.seconds = adjustBuffer[1]*10 + adjustBuffer[0];
                        timeAdjust.highlight[0] = highlightAdjust[0];
                        timeAdjust.highlight[1] = highlightAdjust[1];
                        timeAdjust.highlight[2] = highlightAdjust[2];
                        timeAdjust.highlight[3] = highlightAdjust[3];
                        timeBundle.value = static_cast<void*>(&timeAdjust);
                        xQueueSendToBack(Queues::displayQueue, &timeBundle, portMAX_DELAY);
                    }
                }

                else if(receivedEvent == FROM_TOUCH2)
                {
                    uint32_t now = millis();
                    if(now - lastTouch2ProcessedTime > DEBOUNCE_INTERVAL_MS)
                    {
                        Serial.println("Incrementing digit...");
                        lastTouch2ProcessedTime = now;

                        adjustBuffer[adjustDigitsIndex] = (adjustBuffer[adjustDigitsIndex] + 1) % 10;

                        timeBundle.type = Types::DataType::ADJUST_TIME;
                        Types::StopWatchTimeAdjustment timeAdjust;
                        timeAdjust.minutes = adjustBuffer[3]*10 + adjustBuffer[2];
                        timeAdjust.seconds = adjustBuffer[1]*10 + adjustBuffer[0];
                        timeAdjust.highlight[0] = highlightAdjust[0];
                        timeAdjust.highlight[1] = highlightAdjust[1];
                        timeAdjust.highlight[2] = highlightAdjust[2];
                        timeAdjust.highlight[3] = highlightAdjust[3];
                        timeBundle.value = static_cast<void*>(&timeAdjust);
                        xQueueSendToBack(Queues::displayQueue, &timeBundle, portMAX_DELAY);
                    }
                }

                else if(receivedEvent == FROM_BUTTON)
                {
                    switch(currentPomodoroState)
                    {
                        case Types::PomodoroState::FOCUS:
                            timerDurationFocus[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                            timerDurationFocus[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                            minutes = timerDurationFocus[0];
                            seconds = timerDurationFocus[1];
                            break;
                        case Types::PomodoroState::SHORT_BREAK:
                            timerDurationShortBreak[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                            timerDurationShortBreak[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                            minutes = timerDurationShortBreak[0];
                            seconds = timerDurationShortBreak[1];
                            break;
                        case Types::PomodoroState::LONG_BREAK:
                            timerDurationLongBreak[0] = adjustBuffer[3]*10 + adjustBuffer[2];
                            timerDurationLongBreak[1] = adjustBuffer[1]*10 + adjustBuffer[0];
                            minutes = timerDurationLongBreak[0];
                            seconds = timerDurationLongBreak[1];
                            break;
                    }
                    nextSysState = Types::SystemState::TIMER;
                    xNextWakeTime = xTaskGetTickCount() + xDelay;
                    nextPomodoroState = currentPomodoroState;
                    startTick = true;
                }
            }

            else // Paused
            {   
                xQueueReceive(Queues::interactionEventQueue, &receivedEvent, portMAX_DELAY);
                if (receivedEvent == FROM_BUTTON)
                {
                    nextSysState = Types::SystemState::TIMER;
                    xNextWakeTime = xTaskGetTickCount() + xDelay;
                }
                else
                {
                    nextSysState = Types::SystemState::ADJUST;
                    nextPomodoroState = Types::PomodoroState::FOCUS;
                    startTick = true;
                }
            }

            if(nextSysState != currentSysState || nextPomodoroState != currentPomodoroState)
            {
                Types::StateChangeRequest stateChangeRequest = {
                    nextSysState,
                    nextPomodoroState
                };
                xQueueSendToBack(Queues::systemStateQueue, &stateChangeRequest, 0);
            }

            currentPomodoroState = nextPomodoroState;
            currentSysState = nextSysState;
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
        QueueHandle_t systemStateQueue = Queues::systemStateQueue;

        // Queue set
        QueueSetHandle_t brainQueueSet = xQueueCreateSet(6); 

        xQueueAddToSet(dhtSensorQueue, brainQueueSet);
        xQueueAddToSet(sysMonitorQueue, brainQueueSet);
        xQueueAddToSet(pirSensorQueue, brainQueueSet);
        xQueueAddToSet(ldrSensorQueue, brainQueueSet);
        xQueueAddToSet(cameraInferenceQueue, brainQueueSet);
        xQueueAddToSet(systemStateQueue, brainQueueSet);

        QueueSetMemberHandle_t xActivatedMember;

        // Initial state: ADJUST
        vTaskPrioritySet(handleDisplayTask, 4);
        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
        xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
        ISR::detachPIRISR();

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

                        if(currentSystemState.sysState == Types::SystemState::TIMER)
                            recalculateFocusIndex = true;
                      
                        else if(currentSystemState.sysState == Types::SystemState::ADJUST)
                        {
                            Types::DisplayData displayDataCamOnAdjust;
                            displayDataCamOnAdjust.type = Types::DataType::CAM_DETECTION_ON_ADJUST;
                            displayDataCamOnAdjust.value = static_cast<void*>(&faceDetected);
                            xQueueSendToBack(displayQueue, &displayDataCamOnAdjust, 0);
                        }

                        detectionDebounce = false;
                        debounceCounter = 0;
                        cameraEvents++;
                    }
                }
            }

            if(xActivatedMember == systemStateQueue) 
            {
                if(xQueueReceive(systemStateQueue, &currentSystemState, 0) == pdTRUE)
                {
                    switch(currentSystemState.sysState)
                    {
                        case Types::SystemState::TIMER:
                            Serial.println("System State changed to TIMER");
                            vTaskPrioritySet(handleDisplayTask, 2);
                            if(currentSystemState.pomodoroState == Types::PomodoroState::FOCUS) 
                            {
                                ISR::attachPIRISR();
                                xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
                                xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                            }
                            else 
                            {
                                ISR::detachPIRISR();
                                xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                                xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                            }
                            break;
                        case Types::SystemState::ADJUST:
                            Serial.println("System State changed to ADJUST");
                            vTaskPrioritySet(handleDisplayTask, 4);
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
                            xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                            ISR::detachPIRISR();
                            break;
                        case Types::SystemState::FINISHED:
                            Serial.println("System State changed to FINISHED");
                            xTaskCreatePinnedToCore(vBuzzerTask, "Task buzzer", 1024, NULL, 1, NULL, 1);
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                            ISR::detachPIRISR();
                            focusIndex = 0.0f;
                            motionEvents = 0;
                            cameraEvents = 0;
                            cameraDetections = 0;
                            break;
                        case Types::SystemState::PAUSED:
                            Serial.println("System State changed to PAUSED");
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
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

        int lastMin = -11;
        int lastSec = -11;
        int lastAdjIndex = -1;

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
                    case Types::DataType::UPDATE_TIME:
                    {
                        // Lógica inteligente de dígitos: Só desenha se mudar
                        int newMin = ((Types::StopWatchTime*)displayData.value)->minutes;
                        int newSec = ((Types::StopWatchTime*)displayData.value)->seconds;

                        if (newMin / 10 != lastMin / 10) 
                            Display::vPrintTime(tft, 3, newMin / 10);
                        
                        if (newMin % 10 != lastMin % 10) 
                            Display::vPrintTime(tft, 2, newMin % 10);

                        if (newSec / 10 != lastSec / 10) 
                            Display::vPrintTime(tft, 1, newSec / 10);

                        if (newSec % 10 != lastSec % 10) 
                            Display::vPrintTime(tft, 0, newSec % 10);

                        lastMin = newMin;
                        lastSec = newSec;
                        break;
                    }
                    case Types::DataType::ADJUST_TIME:
                    {
                        int m = (int) ((Types::StopWatchTime*)displayData.value)->minutes;
                        int s = (int) ((Types::StopWatchTime*)displayData.value)->seconds;
                        bool* h = (bool*) ((Types::StopWatchTimeAdjustment*)displayData.value)->highlight;
                        
                        Display::vPrintTimerAdjustment(tft, 3, m / 10, h[3]);
                        Display::vPrintTimerAdjustment(tft, 2, m % 10, h[2]);
                        Display::vPrintTimerAdjustment(tft, 1, s / 10, h[1]);
                        Display::vPrintTimerAdjustment(tft, 0, s % 10, h[0]);
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
                    case Types::DataType::SCREEN_CHANGE_REQUEST:
                    {
                        Types::ScreenChangeRequest screenChange = *(Types::ScreenChangeRequest*)displayData.value;
                        Types::PomodoroState newPomodoroState = screenChange.pomodoroState;
                        Types::SystemState newSystemState = screenChange.systemState;
                        switch (newSystemState)
                        {
                            case Types::SystemState::TIMER:
                                Display::vInitializeTimerDisplay(tft, newPomodoroState);
                                lastMin = -11; 
                                lastSec = -11;
                                break;
                            case Types::SystemState::ADJUST:
                                Display::vPrintAdjustingTimer(tft, newPomodoroState, screenChange.timerCount[0], screenChange.timerCount[1]);
                                lastMin = screenChange.timerCount[0]; 
                                lastSec = screenChange.timerCount[1];
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
                    case Types::DataType::CAM_DETECTION_ON_ADJUST:
                    {
                        bool* camDetect = (bool*) (displayData.value);
                        Display::vPrintFaceDetectedOnAdjustment(tft, *camDetect);
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

        for(;;)
        {
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING | SystemSync::BIT_SYSTEM_ADJUST, pdFALSE, pdTRUE, portMAX_DELAY);

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
            vTaskDelay(pdMS_TO_TICKS(CAMERA_TASK_DELAY_MS));
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

        for(;;) 
        {
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING,pdFALSE, pdTRUE, portMAX_DELAY);
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

            vTaskDelay(pdMS_TO_TICKS(DHT_TASK_DELAY_MS));
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
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING,pdFALSE, pdTRUE, portMAX_DELAY);
            xLastWakeTime = xTaskGetTickCount();
            while(xSemaphoreTake(Semaphores::pirEventSemaphore, 0) == pdTRUE)
            {
                pirValue++;
                Serial.println("PIR Motion Detected!");
            }

            result = pirValue;
            xQueueOverwrite(pirSensorQueue, &result);
            Serial.println("PIR value sent!");
            pirValue = 0;

            // Timing isn't THAT necessary here, but as the output time resolution is taken as 10 seconds for calculations, we keep it.
            vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(PIR_TASK_DELAY_MS));
        }
    }

    void vLDRSensorTask(void* parameter)
    {
        int ldrValue = 0;
        QueueHandle_t ldrSensorQueue = Queues::ldrSensorQueue;

        for(;;)
        {
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING,pdFALSE, pdTRUE, portMAX_DELAY);
            ldrValue = analogRead(PIN_IN_ANLG);
            xQueueOverwrite(ldrSensorQueue, &ldrValue);
            Serial.println("LDR value sent!");
            vTaskDelay(pdMS_TO_TICKS(LDR_TASK_DELAY_MS));
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