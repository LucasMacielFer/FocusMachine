#include "ISR.h"

namespace ISR
{
    void setupISRs()
    {
        attachInterrupt(digitalPinToInterrupt(PIN_IN_PIR), pirISR, RISING);
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressISR, FALLING);
        touchAttachInterruptArg(PIN_TOUCH1, touchISR, (void*) FROM_TOUCH1, 1700);
        touchAttachInterruptArg(PIN_TOUCH2, touchISR, (void*) FROM_TOUCH2, 1700);
    }

    void attachPIRISR()
    {
        attachInterrupt(digitalPinToInterrupt(PIN_IN_PIR), pirISR, RISING);
    }

    void detachPIRISR()
    {
        detachInterrupt(digitalPinToInterrupt(PIN_IN_PIR));
    }

    void IRAM_ATTR pirISR()
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(Semaphores::pirEventSemaphore, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }

    void IRAM_ATTR buttonPressISR()
    {
        int source =  FROM_BUTTON;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(Queues::interactionEventQueue, &source, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
    
    void IRAM_ATTR touchISR(void* arg)
    {
        int source = (int) arg;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(Queues::interactionEventQueue, &source, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
} // namespace ISR
