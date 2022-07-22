#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "camera_utils.h"
#include "sd_utils.h"
#include "wifi_utils.h"
#include "ntp_utils.h"
#include "sockets_utils.h"

TaskHandle_t recording_task_handle = NULL;
TaskHandle_t networking_task_handle = NULL;
SemaphoreHandle_t sd_sempahore = NULL;

//TODO: make sure to delete the next first image if space isnt enough or the first image doesnt exists due to some random deleteing by user
#define BUFFER_MAX_SIZE 500000
char* tcp_buf;
#define PASSWORD "12345678"

void recordingTask(void* data)
{
    camInit();
    // first image is usually bad quality at startup so its discarded
    camera_fb_t *fb =  esp_camera_fb_get();
    ESP_LOGI("Recording Task", "%d %d %d", fb->height, fb->width, fb->len);
    esp_camera_fb_return(fb);
    uint32_t first_image = 1;
    uint32_t last_image = 0;

    if (exists("first.data"))
    {
        int out = readBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
       
    }
    else
    {
        writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);

    }

    if (exists("last.data"))
    {
        int out = readBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);
        
    }
    else
    {
        writeBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);

    }
    

    char img_name[100];
    
    while (1)
    {

        last_image++;
        ESP_LOGI("Recording Task", "first image: %u | last image: %u", first_image, last_image);

        sprintf(img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)last_image);
        if (xSemaphoreTake(sd_sempahore, portMAX_DELAY))
        {
            ESP_LOGI("Recording Task", "sd_semaphore taken succesfully");
            fb =  esp_camera_fb_get();
            ESP_LOGI("Recording Task", "%d %d %d", fb->height, fb->width, fb->len);
            int out = writeBytes(img_name, (void*)fb->buf, 1, fb->len);
            if (out < 0)
            {
                ESP_LOGW("Recording Task", "Retrying in 30s");    // corrective action for sd card return code 109 (https://www.automationdirect.com/microsites/clickplcs/click-help/Content/278.htm)
                esp_camera_fb_return(fb);
                xSemaphoreGive(sd_sempahore);
                last_image--;
                vTaskDelay(30000/portTICK_PERIOD_MS);
                continue;
            }

            if (out < fb->len)
            {
                
                ESP_LOGE("Recording Task", "SD full");

                char first_img_name[100];

                readBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
                sprintf(first_img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)first_image);

                ESP_LOGI("Recording Task", "Deleting: %s to free space", first_img_name);

                remove(first_img_name);

                first_image++;
                writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
                
                last_image--;
                
            }
        

            esp_camera_fb_return(fb);
            writeBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);
            xSemaphoreGive(sd_sempahore);
        }
        esp_camera_fb_return(fb);
        last_image--;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);

}

void clientHandler(void* client_ptr)
{
    tcp_buf = (char*)malloc(BUFFER_MAX_SIZE);
    ESP_LOGI("CLIENT_HANDLER", "1");
    memset(tcp_buf, 0, BUFFER_MAX_SIZE);
    ESP_LOGI("CLIENT_HANDLER", "1");
    int client = *(int*)client_ptr;
    ESP_LOGI("CLIENT_HANDLER", "1");

    free(client_ptr);

    ESP_LOGI("CLIENT_HANDLER", "1");

    recv(client, tcp_buf, 8, 0);
    ESP_LOGI("CLIENT_HANDLER", "recieved from client: %s", tcp_buf);
    if (strncmp(PASSWORD, tcp_buf, 8) == 0)
    {
        
        ESP_LOGI("CLIENT_HANDLER", "Auth success");
        while (1)
        {
            if (xSemaphoreTake(sd_sempahore, 10))
            {
                ESP_LOGI("CLIENT_HANDLER", "sd_semaphore taken succesfully");
                uint32_t latest_image_stored;
                char latest_img_name[100];
                readBytes(MOUNT_POINT"/last.data", (void*)&latest_image_stored, sizeof(uint32_t), 1);
                sprintf(latest_img_name, MOUNT_POINT"/%010d.jpg", (uint32_t)latest_image_stored);
                ESP_LOGI("CLIENT_HANDLER", "sd_semaphore taken succesfully");
                ESP_LOGI("CLIENT_HANDLER", "opening file: %s", latest_img_name);
                
                int ret = readBytes(latest_img_name, (void*)tcp_buf, 1, BUFFER_MAX_SIZE);
                xSemaphoreGive(sd_sempahore);
                if (ret == -1)
                {
                    ESP_LOGE("CLIENT_HANDLER", "Failed to open file: %s", latest_img_name);
                    char send_data[7] = "Failed";
                    send(client, send_data, 7, 0);
                }
                else if (ret > 0) // implement chunking by checking readbytes return later and add fseek to readbytes arguments
                {
                    ESP_LOGI("CLIENT_HANDLER", "Sending %d byte to client", ret);
                    send(client, tcp_buf, ret, 0);
                    
                }
                break;
            }
            vTaskDelay(100/portTICK_PERIOD_MS); 
        }
    }
    else 
    {
        ESP_LOGI("CLIENT_HANDLER", "Auth failed");
        char send_data[7] = "Failed";
        send(client, send_data, 7, 0);
    }
    closesocket(client);
    free(tcp_buf);
    vTaskDelete(NULL);
}


void app_main(void)
{



    vSemaphoreCreateBinary(sd_sempahore);
    if(sd_sempahore == NULL)
    {
        ESP_LOGE("MAIN", "failed to create sd_sempahore");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    sdInit();

    wifiInitSTA(STA_SSID, STA_PASSWORD);
    
    struct tm timeinfo = {0};
    time_t now = 0;
    ntpInit("UTC-1");
    xTaskCreatePinnedToCore(recordingTask, "Recording Task", 8192*4, NULL, 5, &recording_task_handle, 0);
    // sdInit();
    serverInit((void*)clientHandler);

    
}
