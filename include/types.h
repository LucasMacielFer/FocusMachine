#pragma once
#include <cstdint>

namespace Types
{
    // Enum for different types of data that can be displayed
    enum class DataType
    {
        TEMPERATURE,
        HUMIDITY,
        LUMINOSITY,
        TIME,
        FOCUS_INDEX,
        COMFORT_INDEX,
        ADJ_TIME,
        SCREEN_CHANGE_REQUEST
    };

    enum class PomodoroState
    {
        FOCUS,
        SHORT_BREAK,
        LONG_BREAK,
    };

    enum class SystemState
    {
        TIMER,
        ADJUST,
        FINISHED
    };

    typedef struct
    {
        int minutes;
        int seconds;
    } StopWatchTime;

    typedef struct
    {
        float temperature;
        float humidity;
    } DHTSensorData;
    
    typedef struct
    {
        DataType type;
        void* value;
    } DisplayData;

    typedef struct
    {
        uint16_t idleTicks;
        uint16_t totalTicks;
    } SystemMetrics;

    typedef struct
    {
        PomodoroState pomodoroState;
        SystemState systemState;
        int timerCount[2];
    } ScreenChangeRequest;
} // namespace Types