#include <stdio.h>

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "camera_utils.h"
#include "sd_utils.h"
#include "wifi_utils.h"
#include "ntp_utils.h"



void app_main(void)
{
    camInit();

    wifiInitSTA(STA_SSID, STA_PASSWORD);
   
    struct tm timeinfo = {0};
    time_t now = 0;
    ntpInit("UTC-1");

    sdInit();

    // first image is usually bad quality at startup so its discarded
    camera_fb_t *fb =  esp_camera_fb_get();
    ESP_LOGI("MAIN", "%d %d %d", fb->height, fb->width, fb->len);
    esp_camera_fb_return(fb);


    char img_name[100];
    for (int i = 0; i < 2; i++)
    {
        getTime(&now, &timeinfo);

        sprintf(img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)now);
        writeTxt(MOUNT_POINT"/latest.txt", img_name);
        fb =  esp_camera_fb_get();
        ESP_LOGI("MAIN", "%d %d %d", fb->height, fb->width, fb->len);
        writeImg(img_name, fb->buf, fb->len);
        esp_camera_fb_return(fb);
        readTxt(MOUNT_POINT"/latest.txt", img_name);
        ESP_LOGI("MAIN", "latest file name = %s", img_name);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}
