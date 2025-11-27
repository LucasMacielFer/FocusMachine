#include "classification.h"

bool faceDetect(camera_fb_t *fb, float *confidence) 
{
    if (fb == NULL || fb->format != PIXFORMAT_RGB565) {
        return false;
    }

    std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
    std::list<dl::detect::result_t> &results = s2.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, candidates);

    if (results.size() > 0) {
        dl::detect::result_t rosto = results.front();
                if (confidence != NULL) {
            *confidence = rosto.score;
        }
        
        return true;
    }

    if (confidence != NULL) *confidence = 0.0;
    return false;
}