#include "semaphores.h"

namespace Semaphores
{
    SemaphoreHandle_t sysMonitorSemaphore = nullptr;
    SemaphoreHandle_t pirEventSemaphore = nullptr;

    void vCreateSemaphores(void)
    {
        sysMonitorSemaphore = xSemaphoreCreateBinary();
        pirEventSemaphore = xSemaphoreCreateCounting(5, 0);
    }
} // namespace Semaphores