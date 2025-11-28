#include "semaphores.h"

namespace Semaphores
{
    SemaphoreHandle_t displayPomodoroHandshakeSemaphore = nullptr;
    SemaphoreHandle_t pirEventSemaphore = nullptr;
    SemaphoreHandle_t buttonSemaphore = nullptr;
    
    void vCreateSemaphores(void)
    {
        displayPomodoroHandshakeSemaphore = xSemaphoreCreateBinary();
        pirEventSemaphore = xSemaphoreCreateCounting(5, 0);
        buttonSemaphore = xSemaphoreCreateBinary();
    }
} // namespace Semaphores