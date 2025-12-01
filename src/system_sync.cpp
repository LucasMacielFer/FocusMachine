#include "system_sync.h"

namespace SystemSync
{
    EventGroupHandle_t runStateGroup = nullptr;

    void initializeSystemSync()
    {
        runStateGroup = xEventGroupCreate();
        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
        xEventGroupClearBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_ADJUST);
    }
} // namespace SystemSync