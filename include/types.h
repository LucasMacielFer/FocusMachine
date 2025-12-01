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
        UPDATE_TIME,
        ADJUST_TIME,
        FOCUS_INDEX,
        COMFORT_INDEX,
        SCREEN_CHANGE_REQUEST,
        CAM_DETECTION_ON_ADJUST
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
        FINISHED,
        PAUSED
    };

    typedef struct
    {
        int minutes;
        int seconds;
    } StopWatchTime;

    typedef struct
    {
        int minutes;
        int seconds;
        bool highlight[4];
    } StopWatchTimeAdjustment;

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