#include "ISR.h"

namespace ISR
{
    // Function to setup all ISRs
    void setupISRs()
    {
        attachInterrupt(digitalPinToInterrupt(PIN_IN_PIR), pirISR, RISING);
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressISR, FALLING);
        touchAttachInterruptArg(PIN_TOUCH1, touchISR, (void*) FROM_TOUCH1, TOUCH_THRESHOLD_1);
        touchAttachInterruptArg(PIN_TOUCH2, touchISR, (void*) FROM_TOUCH2, TOUCH_THRESHOLD_2);
    }

    // PIR ISR attach/detach functions
    void attachPIRISR()
    {
        attachInterrupt(digitalPinToInterrupt(PIN_IN_PIR), pirISR, RISING);
    }

    void detachPIRISR()
    {
        detachInterrupt(digitalPinToInterrupt(PIN_IN_PIR));
    }

    // ISR called upon PIR detection
    void IRAM_ATTR pirISR()
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Signal the PIR event counter semaphore
        xSemaphoreGiveFromISR(Semaphores::pirEventSemaphore, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }

    // ISR called upon button press
    void IRAM_ATTR buttonPressISR()
    {
        int source = FROM_BUTTON;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(Queues::interactionEventQueue, &source, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
    
    // ISR called upon touch1/touch2 events
    void IRAM_ATTR touchISR(void* arg)
    {
        // Each touch pin passes its identifier as argument
        int source = (int) arg;
        if(!touchInterruptGetLastStatus((int32_t) arg))
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendToBackFromISR(Queues::interactionEventQueue, &source, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken == pdTRUE)
            {
                portYIELD_FROM_ISR();
            }
        }
    }
} // namespace ISR
