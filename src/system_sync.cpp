#include "system_sync.h"

namespace SystemSync
{
    EventGroupHandle_t runStateGroup = nullptr;

    void initializeSystemSync()
    {
        runStateGroup = xEventGroupCreate();
        xEventGroupSetBits(SystemSync::runStateGroup, SystemSync::BIT_SYSTEM_RUNNING);
    }
} // namespace SystemSync