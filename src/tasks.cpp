#include "tasks.h"

/*  Ultimate stack monitoring device 2000 ultra-XPTO
    UBaseType_t stackSobra = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("[DisplayTask] Stack Livre: %u bytes\n", (unsigned int)stackSobra);
*/

namespace Tasks
{
    // Task initializations
    void vTasksInitialize()
    {
        TaskHandle_t handlePomodoroFSMTask;
        TaskHandle_t handleDisplayTask;
        TaskHandle_t handleCameraInferenceTask;
        TaskHandle_t handleDHTSensorTask;
        TaskHandle_t handlePIRSensorTask;
        TaskHandle_t handleLDRSensorTask;

        xTaskCreatePinnedToCore(vTelemetryTask, "BrainTask", 16384, (void*) NULL, 3, NULL, 0);
        xTaskCreatePinnedToCore(vDisplayTask, "DisplayTask", 8192, NULL, 2, &handleDisplayTask, 0);
        xTaskCreatePinnedToCore(vCameraInferenceTask, "CameraInferenceTask", 32768, NULL, 1, &handleCameraInferenceTask, 1);
        xTaskCreatePinnedToCore(vDHTSensorTask, "DHTSensorTask", 4096, NULL, 1, &handleDHTSensorTask, 0);
        xTaskCreatePinnedToCore(vPIRSensorTask, "PIRSensorTask", 4096, NULL, 1, &handlePIRSensorTask, 0);
        xTaskCreatePinnedToCore(vLDRSensorTask, "LDRSensorTask", 4096, NULL, 1, &handleLDRSensorTask, 0);

        TaskHandle_t* handlers[5] = {&handleDisplayTask, &handleCameraInferenceTask, &handleDHTSensorTask, &handlePIRSensorTask, &handleLDRSensorTask};
        xTaskCreatePinnedToCore(vPomodoroFSMTask, "PomodoroFSMTask", 8192, handlers, 4, NULL, 0);
        esp_register_freertos_idle_hook(Tasks::vSysMonitorIdleHook);
    }

    // Acts as the "Brain Task" of the system, managing the Pomodoro FSM
    void vPomodoroFSMTask(void* parameter)
    {
        // Not all handles are used, but kept for future reference
        TaskHandle_t handleDisplayTask;
        TaskHandle_t handleCameraInferenceTask;
        TaskHandle_t handleDHTSensorTask;
        TaskHandle_t handlePIRSensorTask;
        TaskHandle_t handleLDRSensorTask;

        TaskHandle_t** handlers = (TaskHandle_t**) parameter;
        handleDisplayTask = *(handlers[0]);
        handleCameraInferenceTask = *(handlers[1]);
        handleDHTSensorTask = *(handlers[2]);
        handlePIRSensorTask = *(handlers[3]);
        handleLDRSensorTask = *(handlers[4]);
        
        // Default durations for showcase purposes
        int timerDurationFocus[2] = {0, 25};       // 25 seconds
        int timerDurationShortBreak[2] = {0, 10};  // 5 seconds
        int timerDurationLongBreak[2] = {0, 15};   // 15 seconds

        // Default durations for actual use
        //int timerDurationFocus[2] = {25, 0};       // 25 minutes
        //int timerDurationShortBreak[2] = {10, 0};  // 5 minutes
        //int timerDurationLongBreak[2] = {15, 0};   // 15 minutes


        int shortBreakCounter = 0;  // Counts 3 short breaks before a long break
        Types::StopWatchTime currentTime = { -1, -1 };

        int minutes = 0;
        int seconds = 0;
        bool startTick = true;

        int adjustDigitsIndex = 3;
        int adjustBuffer[4] = {0, 0, 0, 0};
        bool highlightAdjust[4] = {false, false, false, false};

        // Timing variables
        const TickType_t xDelay = pdMS_TO_TICKS(1000);

        int receivedEvent = -1;

        // Debounce variables for touch inputs
        uint32_t lastTouch1ProcessedTime = 0;
        uint32_t lastTouch2ProcessedTime = 0;
        uint32_t nextInputAllowedTime = 0;

        // FSM logic
        Types::PomodoroState currentPomodoroState = Types::PomodoroState::FOCUS;
        Types::PomodoroState nextPomodoroState = currentPomodoroState;
        Types::SystemState currentSysState = Types::SystemState::ADJUST;
        Types::SystemState nextSysState = currentSysState;
        
        // Initial state: Adjust
        vTaskPrioritySet(handleDisplayTask, 4);
        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
        xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);

        Types::StateData stateData;
        Types::DisplayData bundle;
        Types::DisplayData timeBundle;

        timeBundle.type = Types::DataType::UPDATE_TIME;

        xQueueSendToBack(Queues::systemStateQueue, &nextSysState, portMAX_DELAY);
        xSemaphoreTake(Semaphores::displayPomodoroHandshakeSemaphore, portMAX_DELAY);
        TickType_t xLastWakeTime = xTaskGetTickCount();

        for(;;)
        {
            nextSysState = currentSysState;
            nextPomodoroState = currentPomodoroState;

            // TIMER LOGIC
            if(currentSysState == Types::SystemState::TIMER)
            {
                // Start ticking only once upon state entry
                if(startTick)
                {
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
                    xLastWakeTime = xTaskGetTickCount();
                    startTick = false;
                }
                
                vTaskDelayUntil(&xLastWakeTime, xDelay);

                while (xQueueReceive(Queues::interactionEventQueue, &receivedEvent, 0) == pdTRUE)
                {
                    if (receivedEvent == FROM_BUTTON) 
                    {
                        nextSysState = Types::SystemState::PAUSED;
                        break;
                    }
                }

                if(nextSysState == Types::SystemState::TIMER)
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
                }
                
                if (minutes <= 0 && seconds <= 0) 
                {
                    nextSysState = Types::SystemState::FINISHED;
                }
            }

            // FINISHED LOGIC - When the timer reaches 0
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
                xQueueReceive(Queues::interactionEventQueue, &receivedEvent, portMAX_DELAY);
                
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
                startTick = true;
            }

            // ADJUST LOGIC
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

                    // Touch debounce on state change
                    if(now < nextInputAllowedTime)
                        continue;

                    if(now - lastTouch1ProcessedTime > DEBOUNCE_INTERVAL_MS)
                    {
                        lastTouch1ProcessedTime = now;
                        if(adjustDigitsIndex > 0)
                        {
                            highlightAdjust[adjustDigitsIndex] = false;
                            adjustDigitsIndex--;
                            highlightAdjust[adjustDigitsIndex] = true;
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

                            // Touch debounce on state change
                            nextInputAllowedTime = millis() + 4000;
                        }
                    }
                }

                else if(receivedEvent == FROM_TOUCH2)
                {
                    uint32_t now = millis();
                    if(now < nextInputAllowedTime)
                        continue;

                    if(now - lastTouch2ProcessedTime > DEBOUNCE_INTERVAL_MS)
                    {
                        lastTouch2ProcessedTime = now;

                        adjustBuffer[adjustDigitsIndex] = adjustDigitsIndex==0 || adjustDigitsIndex==2 ? (adjustBuffer[adjustDigitsIndex] + 1) % 10 : (adjustBuffer[adjustDigitsIndex] + 1) % 6;

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
                    nextPomodoroState = currentPomodoroState;
                    startTick = true;
                }
            }

            else // PAUSED LOGIC
            {   
                xQueueReceive(Queues::interactionEventQueue, &receivedEvent, portMAX_DELAY);
                if (receivedEvent == FROM_BUTTON)
                {
                    nextSysState = Types::SystemState::TIMER;
                    xLastWakeTime = xTaskGetTickCount();
                }
                else
                {
                    nextSysState = Types::SystemState::ADJUST;
                    nextPomodoroState = Types::PomodoroState::FOCUS;
                    startTick = true;
                    nextInputAllowedTime = millis() + 4000;
                }
            }

            // State change handling
            if(nextSysState != currentSysState || nextPomodoroState != currentPomodoroState)
            {
                switch(nextSysState)
                {
                    case Types::SystemState::TIMER:
                        vTaskPrioritySet(handleDisplayTask, 2);
                        if(nextPomodoroState == Types::PomodoroState::FOCUS) 
                        {
                            ISR::attachPIRISR();
                            xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
                        }
                        else 
                        {
                            ISR::detachPIRISR();
                            xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                        }
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                        break;
                    case Types::SystemState::ADJUST:
                        vTaskPrioritySet(handleDisplayTask, 4);
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING );
                        xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                        ISR::detachPIRISR();
                        break;
                    case Types::SystemState::FINISHED:
                        xTaskCreatePinnedToCore(vBuzzerTask, "Task buzzer", 1024, NULL, 1, NULL, 1);
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                        ISR::detachPIRISR();
                        break;
                    case Types::SystemState::PAUSED:
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
                        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
                        ISR::detachPIRISR();
                        break;
                    default:
                        break;
                }

                stateData.pomodoroState = nextPomodoroState;
                stateData.systemState = nextSysState;
                xQueueSendToBack(Queues::systemStateQueue, &stateData, portMAX_DELAY);

                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.println("\n\nState changed!");
                Serial.print("System State: ");
                switch(nextSysState)
                {
                    case Types::SystemState::ADJUST:
                        Serial.println("ADJUST");
                        break;
                    case Types::SystemState::TIMER:
                        Serial.println("TIMER");
                        break;
                    case Types::SystemState::PAUSED:
                        Serial.println("PAUSED");
                        break;
                    case Types::SystemState::FINISHED:
                        Serial.println("FINISHED");
                        break;
                    default:
                        break;
                }
                Serial.print("Pomodoro State: ");
                switch(nextPomodoroState)
                {
                    case Types::PomodoroState::FOCUS:
                        Serial.println("FOCUS");
                        break;
                    case Types::PomodoroState::SHORT_BREAK:
                        Serial.println("SHORT_BREAK");
                        break;
                    case Types::PomodoroState::LONG_BREAK:
                        Serial.println("LONG_BREAK");
                        break;
                    default:
                        break;
                }
                Serial.print("\n\n");
                xSemaphoreGive(Semaphores::serialMutex);
            }

            currentPomodoroState = nextPomodoroState;
            currentSysState = nextSysState;
        }
    }
    
    // Telemetry Task - Collects data from various sensors and computes indices
    void vTelemetryTask(void* parameter)
    {
        Types::DHTSensorData dhtData;
        Types::SystemMetrics sysMetrics;
        Types::StateData stateData;

        Algorithms::ComfortConfig comfortConfig = {
            .idealTemp = 23.0f, .tempTol = 7.0f,
            .idealHum = 50.0f,  .humTol = 25.0f,
            .luxIdealMin = 1800, .luxIdealMax = 3200, .luxTol = 1000,
            .pirMaxEvents = 3
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
        bool adjustingCamera = false;

        // Queue set
        QueueSetHandle_t telemetryQueueSet = xQueueCreateSet(8); 

        xQueueAddToSet(Queues::dhtSensorQueue, telemetryQueueSet);
        xQueueAddToSet(Queues::sysMonitorQueue, telemetryQueueSet);
        xQueueAddToSet(Queues::pirSensorQueue, telemetryQueueSet);
        xQueueAddToSet(Queues::ldrSensorQueue, telemetryQueueSet);
        xQueueAddToSet(Queues::cameraInferenceQueue, telemetryQueueSet);
        xQueueAddToSet(Queues::systemStateQueue, telemetryQueueSet);
        QueueSetMemberHandle_t xActivatedMember;

        for(;;)
        {
            // Wait indefinitely for any of the queues to have data
            xActivatedMember = xQueueSelectFromSet(telemetryQueueSet, portMAX_DELAY);

                
            if(xQueueReceive(Queues::systemStateQueue, &stateData, 0) == pdTRUE)
            {
                if(stateData.systemState == Types::SystemState::FINISHED) 
                {
                    motionEvents = 0;
                    cameraEvents = 0;
                    cameraDetections = 0;
                    recalculateComfortIndex = false;
                    recalculateFocusIndex = false;
                    xQueueReset(Queues::pirSensorQueue);
                    xQueueReset(Queues::dhtSensorQueue);
                    xQueueReset(Queues::ldrSensorQueue);
                    if(stateData.pomodoroState == Types::PomodoroState::FOCUS) 
                    {
                        Types::DisplayData displayDataFocusIndex;
                        displayDataFocusIndex.type = Types::DataType::FOCUS_INDEX_UPON_END;
                        displayDataFocusIndex.value = static_cast<void*>(&focusIndex);
                        xQueueSendToBack(Queues::displayQueue, &displayDataFocusIndex, 0);
                    }
                }

                xQueueReset(Queues::cameraInferenceQueue);
            }

            // Processes DHT sensor data
            if(xQueueReceive(Queues::dhtSensorQueue, &dhtData, 0) == pdTRUE)
            {
                Types::DisplayData displayDataTemp;
                Types::DisplayData displayDataHum;
                displayDataTemp.type = Types::DataType::TEMPERATURE;
                displayDataTemp.value = static_cast<void*>(&dhtData.temperature);
                displayDataHum.type = Types::DataType::HUMIDITY;
                displayDataHum.value = static_cast<void*>(&dhtData.humidity);

                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.print("Temperature: ");
                Serial.print(dhtData.temperature);
                Serial.print(" °C, Humidity: ");
                Serial.print(dhtData.humidity);
                Serial.println(" %");
                xSemaphoreGive(Semaphores::serialMutex);

                if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_RUNNING)
                {
                    xQueueSendToBack(Queues::displayQueue, &displayDataTemp, 0);
                    xQueueSendToBack(Queues::displayQueue, &displayDataHum, 0);
                }
                recalculateComfortIndex = true;
            }

            // Processes PIR sensor data
            if(xQueueReceive(Queues::pirSensorQueue, &PIRdetections, 0) == pdTRUE)
            {
                motionEvents += PIRdetections;
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.print("PIR Detections: ");
                Serial.println(PIRdetections);
                xSemaphoreGive(Semaphores::serialMutex);
                recalculateComfortIndex = true;
            }
    

            // Processes LDR sensor data
            if(xQueueReceive(Queues::ldrSensorQueue, &luminosity, 0) == pdTRUE)
            {
                Types::DisplayData displayDataLdr;
                displayDataLdr.type = Types::DataType::LUMINOSITY;

                luminosityPercentage = (static_cast<float>(luminosity) / 4095.0f) * 100.0f;

                displayDataLdr.value = static_cast<void*>(&luminosityPercentage);

                if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_RUNNING)
                    xQueueSendToBack(Queues::displayQueue, &displayDataLdr, 0);

                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.print("Luminosity: ");
                Serial.println(luminosity);
                xSemaphoreGive(Semaphores::serialMutex);
                recalculateComfortIndex = true;
            }

            // Processes camera inference results
            if(xQueueReceive(Queues::cameraInferenceQueue, &faceDetected, 0) == pdTRUE)
            {
                if(faceDetected) 
                    detectionDebounce = true;

                debounceCounter++;

                if(debounceCounter >= CAMERA_DEBOUNCE_THRESHOLD) 
                {
                    if(detectionDebounce) 
                    {
                        cameraDetections++;
                    }

                    if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_RUNNING)
                        recalculateFocusIndex = true;
                    
                    else if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_ADJUST)
                    {
                        Types::DisplayData displayDataCamOnAdjust;
                        displayDataCamOnAdjust.type = Types::DataType::CAM_DETECTION_ON_ADJUST;
                        displayDataCamOnAdjust.value = static_cast<void*>(&faceDetected);
                        xQueueSendToBack(Queues::displayQueue, &displayDataCamOnAdjust, 0);
                    }

                    detectionDebounce = false;
                    debounceCounter = 0;
                    cameraEvents++;
                }
            }

            // Read system metrics
            if(xQueueReceive(Queues::sysMonitorQueue, &sysMetrics, 0) == pdTRUE)
            {
                if(sysMetrics.totalTicks > 0)
                {
                    cpuUsage = 100.0f * (1.0f - ((float)sysMetrics.idleTicks / (float)sysMetrics.totalTicks));
                    
                    if (cpuUsage < 0.0f) cpuUsage = 0.0f;
                    if (cpuUsage > 100.0f) cpuUsage = 100.0f;

                    xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                    Serial.print("CPU Usage: ");
                    Serial.print(cpuUsage);
                    Serial.println(" %");
                    xSemaphoreGive(Semaphores::serialMutex);
                }
                else
                {
                    xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                    Serial.println("Error: TotalTicks is 0");
                    xSemaphoreGive(Semaphores::serialMutex);
                }
            }

            // Calculates comfort index if needed
            if(recalculateComfortIndex)
            {
                comfortIndex = Algorithms::calculateComfortIndex(dhtData.temperature, dhtData.humidity, luminosity, motionEvents, comfortConfig);
                Types::DisplayData displayDataComfort;
                displayDataComfort.type = Types::DataType::COMFORT_INDEX;
                displayDataComfort.value = static_cast<void*>(&comfortIndex);

                if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_RUNNING)
                    xQueueSendToBack(Queues::displayQueue, &displayDataComfort, 0);

                recalculateComfortIndex = false;
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.print("Comfort Index: ");
                Serial.println(comfortIndex);
                xSemaphoreGive(Semaphores::serialMutex);
            }

            // Calculates focus index if needed
            if(recalculateFocusIndex)
            {
                focusIndex = Algorithms::calculateFocusIndex(cameraDetections, cameraEvents);
                Types::DisplayData displayDataFocus;
                displayDataFocus.type = Types::DataType::FOCUS_INDEX;
                displayDataFocus.value = static_cast<void*>(&focusIndex);

                if(xEventGroupGetBits(SystemSync::runStateGroup) & SystemSync::BIT_SYSTEM_RUNNING)
                    xQueueSendToBack(Queues::displayQueue, &displayDataFocus, 0);
                
                recalculateFocusIndex = false;
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.print("Focus Index: ");
                Serial.println(focusIndex);
                xSemaphoreGive(Semaphores::serialMutex);
            }
        }
    }

    // Display Task - Manages all display updates
    void vDisplayTask(void* parameter)
    {
        QueueHandle_t myQueue = Queues::displayQueue;
        Types::DisplayData displayData;

        float* floatBuffer = nullptr;

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
                // Handle display updates based on the type of data received
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
                        // Only update digits that have changed
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
                    case Types::DataType::FOCUS_INDEX_UPON_END:
                    {
                        floatBuffer = (float*) (displayData.value);
                        Display::vPrintFocusIndexUponFocusEnd(tft, *floatBuffer);
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

    // Camera Inference Task - Captures images and performs face detection
    void vCameraInferenceTask(void* parameter)
    {
        camera_fb_t * fb;
        float acc = 0.0;
        bool detected = false;

        for(;;)
        {
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING | SystemSync::BIT_SYSTEM_ADJUST, pdFALSE, pdFALSE, portMAX_DELAY);

            fb = esp_camera_fb_get();
            if(fb)
                detected = Classification::faceDetect(fb, &acc);
            else
                continue;

            if(detected) 
            {
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.printf("FACE DETECTED! Accuracy: %.2f%%\n", acc * 100);
                xSemaphoreGive(Semaphores::serialMutex);
            }
            else 
            {
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.println("No face detected...");
                xSemaphoreGive(Semaphores::serialMutex);
            }

            xQueueOverwrite(Queues::cameraInferenceQueue, &detected);
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(CAMERA_TASK_DELAY_MS));
        }
    }

    // DHT Sensor Task - Reads temperature and humidity from DHT22 sensor
    void vDHTSensorTask(void* parameter)
    {
        Types::DHTSensorData dhtData;
        dhtData.temperature = 0.0f;
        dhtData.humidity = 0.0f;

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

            if(isnan(h) || isnan(t)) 
            {
                xSemaphoreTake(Semaphores::serialMutex, portMAX_DELAY);
                Serial.println("Failed to read from DHT!");
                xSemaphoreGive(Semaphores::serialMutex);
            } 
            else 
            {
                dhtData.humidity = h;
                dhtData.temperature = t;
                xQueueOverwrite(dhtSensorQueue, &dhtData);
            }

            vTaskDelay(pdMS_TO_TICKS(DHT_TASK_DELAY_MS));
        }
    }

    // PIR Sensor Task - Counts motion events from PIR sensor
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

            // Empties the event counting semaphore to count all events since last read
            while(xSemaphoreTake(Semaphores::pirEventSemaphore, 0) == pdTRUE)
            {
                pirValue++;
            }

            result = pirValue;
            xQueueOverwrite(pirSensorQueue, &result);
            pirValue = 0;

            // Timing isn't THAT necessary here, but as the output time resolution is taken as 10 seconds for calculations, we keep it.
            vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(PIR_TASK_DELAY_MS));
        }
    }

    // LDR Sensor Task - Reads luminosity from LDR sensor
    void vLDRSensorTask(void* parameter)
    {
        int ldrValue = 0;
        QueueHandle_t ldrSensorQueue = Queues::ldrSensorQueue;

        for(;;)
        {
            xEventGroupWaitBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING,pdFALSE, pdTRUE, portMAX_DELAY);
            ldrValue = analogRead(PIN_IN_ANLG);
            xQueueOverwrite(ldrSensorQueue, &ldrValue);
            vTaskDelay(pdMS_TO_TICKS(LDR_TASK_DELAY_MS));
        }
    }

    // Buzzer Task - Sounds buzzer when timer finishes
    void vBuzzerTask(void* parameter)
    {
        for(int i = 0; i < 5; i++) 
        {
            ledcWrite(0, 250); // 50% duty cycle
            vTaskDelay(pdMS_TO_TICKS(100));
            ledcWrite(0, 0);   // Turn off
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelete(NULL);
    }

    // System Monitor Idle Hook - Monitors idle time for CPU usage calculation
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
            msg.totalTicks = totalTimePassed;

            // It's not an ISR, I know, but using this function avoids headaches
            xQueueSendFromISR(Queues::sysMonitorQueue, &msg, NULL);

            localIdleCounter = 0;
            measurementStartTick = currentTick;
        }

        return true;
    }
}