#pragma once
#include <algorithm>

namespace Algorithms 
{
    struct ComfortConfig
    {
        // Temperature
        float idealTemp;    // Ex: 22.0
        float tempTol;      // Ex: 5.0 from 17 up to 27 before zeroing

        // Humidity (%)
        float idealHum;     // Ex: 45.0
        float humTol;       // Ex: 20.0

        // Luminosity (ADC Raw 0-4095)
        int luxIdealMin;    // Ex: 1500 (Not too dark)
        int luxIdealMax;    // Ex: 3000 (Not too bright)
        int luxTol;         // Ex: 1000 (Tolerance margin outside the ideal)

        // Motion (Events in 10s)
        int pirMaxEvents;   // Ex: 4 (The value we consider "Total Chaos")
    };

    // Function to calculate comfort index based on temperature, humidity, luminosity, and motion events
    float calculateComfortIndex(float temperature, float humidity, int luminosity, int motionEvents, ComfortConfig cfg);

    // Function to calculate focus index based on camera detections and events
    float calculateFocusIndex(int cameraDetections, int cameraEvents);
}  // namespace Algorithms