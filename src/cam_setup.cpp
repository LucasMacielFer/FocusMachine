#include "cam_setup.h"

namespace Camera
{
    camera_config_t camera_init() 
    {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Y2_GPIO_NUM;
        config.pin_d1 = Y3_GPIO_NUM;
        config.pin_d2 = Y4_GPIO_NUM;
        config.pin_d3 = Y5_GPIO_NUM;
        config.pin_d4 = Y6_GPIO_NUM;
        config.pin_d5 = Y7_GPIO_NUM;
        config.pin_d6 = Y8_GPIO_NUM;
        config.pin_d7 = Y9_GPIO_NUM;
        config.pin_xclk = XCLK_GPIO_NUM;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        config.xclk_freq_hz = 10000000;
        config.frame_size = FRAMESIZE_QVGA;
        //config.pixel_format = PIXFORMAT_JPEG; // for streaming
        config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = 50;
        config.fb_count = 2;

        return config;
    }

    sensor_t* startCamera()
    {
        camera_config_t config = camera_init();

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) 
        {
          Serial.printf("Camera init failed with error 0x%x", err);
          return nullptr;
        }

        sensor_t * s = esp_camera_sensor_get();
        s->set_vflip(s, 1);        //1-Upside down, 0-No operation
        s->set_hmirror(s, 0);      //1-Reverse left and right, 0-No operation
        s->set_brightness(s, 1);   //up the blightness just a bit
        s->set_saturation(s, -1);  //lower the saturation

        return s;
    }
} // namespace Camera
