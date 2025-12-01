#include "semaphores.h"

namespace Semaphores
{
    SemaphoreHandle_t displayPomodoroHandshakeSemaphore = nullptr;
    SemaphoreHandle_t pirEventSemaphore = nullptr;

    void vCreateSemaphores(void)
    {
        displayPomodoroHandshakeSemaphore = xSemaphoreCreateBinary();
        pirEventSemaphore = xSemaphoreCreateCounting(5, 0);
    }
} // namespace Semaphores