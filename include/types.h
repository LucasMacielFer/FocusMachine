#pragma once
#include <cstdint>

namespace Types
{
    // Different data types for display updates
    enum class DataType
    {
        TEMPERATURE,
        HUMIDITY,
        LUMINOSITY,
        UPDATE_TIME,
        ADJUST_TIME,
        FOCUS_INDEX,
        FOCUS_INDEX_UPON_END,
        COMFORT_INDEX,
        SCREEN_CHANGE_REQUEST,
        CAM_DETECTION_ON_ADJUST
    };

    // Possible states of the Pomodoro timer
    enum class PomodoroState
    {
        FOCUS,
        SHORT_BREAK,
        LONG_BREAK,
    };

    // Possible states of the system
    enum class SystemState
    {
        TIMER,
        ADJUST,
        FINISHED,
        PAUSED
    };

    // Struct to represent time in the stopwatch
    typedef struct
    {
        int minutes;
        int seconds;
    } StopWatchTime;

    // Struct for stopwatch time adjustment with highlight
    typedef struct
    {
        int minutes;
        int seconds;
        bool highlight[4];
    } StopWatchTimeAdjustment;

    // Struct for DHT sensor data
    typedef struct
    {
        float temperature;
        float humidity;
    } DHTSensorData;
    
    // Struct for data to be sent to the display queue
    typedef struct
    {
        DataType type;
        void* value;
    } DisplayData;

    // Struct for system metrics (idle task hook)
    typedef struct
    {
        uint16_t idleTicks;
        uint16_t totalTicks;
    } SystemMetrics;    

    // Struct for screen change requests
    typedef struct
    {
        PomodoroState pomodoroState;
        SystemState systemState;
        int timerCount[2];
    } ScreenChangeRequest;

    // Struct to represent the current state of the system
    typedef struct
    {
        PomodoroState pomodoroState;
        SystemState systemState;
    } StateData;
    
} // namespace Types