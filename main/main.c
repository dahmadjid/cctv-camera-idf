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

TaskHandle_t recording_task_handle = NULL;
TaskHandle_t networking_task_handle = NULL;

void recordingTask(void* data)
{
    camInit();
    sdInit();
    // first image is usually bad quality at startup so its discarded
    camera_fb_t *fb =  esp_camera_fb_get();
    ESP_LOGI("MAIN", "%d %d %d", fb->height, fb->width, fb->len);
    esp_camera_fb_return(fb);
    uint32_t first_image = 1;
    uint32_t last_image = 0;

    if (exists("first.data"))
    {
        int out = readBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
        if (out != 1)
        {
            first_image = 1;
            writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
        }
    }
    else
    {
        writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);

    }

    if (exists("last.data"))
    {
        int out = readBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);
        if (out != 1)
        {
            last_image = 0;
            writeBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);

        }
    }
    else
    {
        writeBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);

    }
    

    char img_name[100];
    
    while (1)
    {

        last_image++;
        ESP_LOGI("MAIN", "first image: %u | last image: %u", first_image, last_image);

        sprintf(img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)last_image);
        
        fb =  esp_camera_fb_get();
        ESP_LOGI("MAIN", "%d %d %d", fb->height, fb->width, fb->len);
        int out = writeBytes(img_name, (void*)fb->buf, 1, fb->len);
        if (out == -1)
        {
            ESP_LOGW("MAIN", "Retrying writing image in a 30s");
            vTaskDelay(30000/portTICK_PERIOD_MS);
            
            out = writeBytes(img_name, (void*)fb->buf, 1, fb->len);
            if (out == -1)
            {
                ESP_LOGE("MAIN", "skipping this image");
                last_image--;
                esp_camera_fb_return(fb);
                vTaskDelay(30000/portTICK_PERIOD_MS);
                continue;
            }
        }

        if (out < fb->len)
        {
            
            ESP_LOGE("MAIN", "SD full");

            char first_img_name[100];

            readBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
            sprintf(first_img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)first_image);

            ESP_LOGI("MAIN", "Deleting: %s to free space", first_img_name);

            remove(first_img_name);

            first_image++;
            writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
            
            last_image--;
            
        }
        

        esp_camera_fb_return(fb);


        writeBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);

}
void app_main(void)
{

    wifiInitSTA(STA_SSID, STA_PASSWORD);
   
    struct tm timeinfo = {0};
    time_t now = 0;
    ntpInit("UTC-1");
    xTaskCreatePinnedToCore(recordingTask, "Recording Task", 8192*4, NULL, 5, &recording_task_handle, 0);

    
}
