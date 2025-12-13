#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

namespace SystemSync
{
    /* This event group is used to manage the system's run and adjust states
    *  in order to synchronize tasks accordingly. I used this approach instead
    *  of vTaskSuspend beacuse it is error-prone and can lead to deadlocks.
    */
    extern EventGroupHandle_t runStateGroup;
    const EventBits_t BIT_SYSTEM_RUNNING = (1 << 0);
    const EventBits_t BIT_SYSTEM_ADJUST = (1 << 1);

    // Event group initialization function
    void initializeSystemSync();
} // namespace SystemSync