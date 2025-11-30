#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

namespace SystemSync
{
    extern EventGroupHandle_t runStateGroup;
    const EventBits_t BIT_SYSTEM_RUNNING = (1 << 0);
    const EventBits_t BIT_SYSTEM_ADJUST = (1 << 1);

    void initializeSystemSync();
} // namespace SystemSync