#pragma once

#include "esp_camera.h"
#include <Arduino.h>
#include <list>
#include "human_face_detect_msr01.hpp"  // Detector Stage 1
#include "human_face_detect_mnp01.hpp"  // Refinador Stage 2

namespace Classification
{
    // Model declarations
    static HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.5F);
    static HumanFaceDetectMNP01 s2(0.2F, 0.3F, 5);

    // Function to perform face detection
    bool faceDetect(camera_fb_t *fb, float *confidence);
} // namespace classification