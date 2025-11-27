#include "semaphores.h"

namespace Semaphores
{
    SemaphoreHandle_t sysMonitorSemaphore = nullptr;

    void vCreateSemaphores(void)
    {
        sysMonitorSemaphore = xSemaphoreCreateBinary();
    }
} // namespace Semaphores