#include "queues.h"

namespace Queues
{
    QueueHandle_t sysMonitorQueue = nullptr;
    QueueHandle_t displayQueue = nullptr;
    QueueHandle_t dhtSensorQueue = nullptr;
    QueueHandle_t pirSensorQueue = nullptr;
    QueueHandle_t ldrSensorQueue = nullptr;
    QueueHandle_t cameraInferenceQueue = nullptr;
    QueueHandle_t interactionEventQueue = nullptr;
    QueueHandle_t systemStateQueue = nullptr;

    // Queue initializations
    void vCreateQueues(void)
    {
        sysMonitorQueue = xQueueCreate(1, sizeof(Types::SystemMetrics));
        displayQueue = xQueueCreate(10, sizeof(Types::DisplayData));
        dhtSensorQueue = xQueueCreate(1, sizeof(Types::DHTSensorData));
        pirSensorQueue = xQueueCreate(1, sizeof(int));
        ldrSensorQueue = xQueueCreate(1, sizeof(int));
        cameraInferenceQueue = xQueueCreate(1, sizeof(bool));
        interactionEventQueue = xQueueCreate(3, sizeof(int));
        systemStateQueue = xQueueCreate(3, sizeof(Types::StateData));
    }
} // namespace Queues