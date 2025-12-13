#include "algorithms.h"

namespace Algorithms 
{
    // Function to calculate comfort index based on temperature, humidity, luminosity, and motion events
    float calculateComfortIndex(float temp, float hum, int adcLight, int pirEvents, ComfortConfig cfg)
    {
        // Temperature e Humidity (Triangular Curve / Linear Decay)
        auto calcLinearScore = [](float val, float ideal, float tol) -> float {
            float diff = abs(val - ideal);
            if (diff >= tol) return 0.0f;
            return 1.0f - (diff / tol);
        };

        float scoreTemp = calcLinearScore(temp, cfg.idealTemp, cfg.tempTol);
        float scoreHum  = calcLinearScore(hum, cfg.idealHum, cfg.humTol);

        // Luminosity (Window with Plateau)
        // We want a score of 1.0 IF it is BETWEEN min and max. If it goes out, it decays.
        float scoreLux = 0.0f;
        if (adcLight >= cfg.luxIdealMin && adcLight <= cfg.luxIdealMax) {
            scoreLux = 1.0f; // Perfect
        } else {
            // Calculate how far it is from the nearest edge
            int dist = 0;
            if (adcLight < cfg.luxIdealMin) dist = cfg.luxIdealMin - adcLight;
            else dist = adcLight - cfg.luxIdealMax;
            
            // Apply tolerance
            if (dist >= cfg.luxTol) scoreLux = 0.0f;
            else scoreLux = 1.0f - ((float)dist / cfg.luxTol);
        }

        // Motion (Inverse: More motion = Less comfort)
        // If pirEvents is 0 -> Score 1.0
        // If pirEvents is Max -> Score 0.0
        float scoreMotion = 1.0f - ((float)pirEvents / (float)cfg.pirMaxEvents);
        
        // Ensure it doesn't go negative if the sensor goes crazy and gives more than max events
        if (scoreMotion < 0.0f) scoreMotion = 0.0f; 

        // Final Weighted Calculation 
        const float wTemp = 0.35f;
        const float wHum  = 0.15f;
        const float wLux  = 0.25f;
        const float wMot  = 0.25f;
        float finalIndex = (scoreTemp * wTemp) + 
                        (scoreHum  * wHum) + 
                        (scoreLux  * wLux) + 
                        (scoreMotion * wMot);

        return finalIndex * 100.0f;
    }

    // Basically, it's the percentage of successful detections over total events
    float calculateFocusIndex(int cameraDetections, int cameraEvents)
    {
        float focusIndex = 0.0f;
        if (cameraEvents > 0)
        {
            focusIndex = (static_cast<float>(cameraDetections) / static_cast<float>(cameraEvents)) * 100.0f;
        }
        return focusIndex;
    }
}  // namespace Algorithms