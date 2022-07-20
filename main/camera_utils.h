#ifndef CAM_UTILS_H
#define CAM_UTILS_H

#include "esp_camera.h"
#include "esp_log.h"


#define BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define image_tag "cam"
static const camera_config_t camera_config = {
    CAM_PIN_PWDN,
    CAM_PIN_RESET,
    CAM_PIN_XCLK,
    CAM_PIN_SIOD,
    CAM_PIN_SIOC,

    CAM_PIN_D7,
    CAM_PIN_D6,
    CAM_PIN_D5,
    CAM_PIN_D4,
    CAM_PIN_D3,
    CAM_PIN_D2,
    CAM_PIN_D1,
    CAM_PIN_D0,
    CAM_PIN_VSYNC,
    CAM_PIN_HREF,
    CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    20000000,
    LEDC_TIMER_0,
    LEDC_CHANNEL_0,

    PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    FRAMESIZE_SVGA,  //QQVGA-UXGAQVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA Do not use sizes above QVGA when not JPEG

    12, //0-63 lower number means higher quality
    1   //if more than one, i2s runs in continuous mode. Use only with JPEG
};

void camInit()
{
    //initialize the camera
    esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(image_tag, "Camera Init Failed");
        ESP_ERROR_CHECK(err);
    }

    return;
}

#endif