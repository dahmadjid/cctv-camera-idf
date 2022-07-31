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

#include "index_other.h"
#include "css.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

extern esp_ip4_addr_t sta_ip;

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;


TaskHandle_t recording_task_handle = NULL;
TaskHandle_t networking_task_handle = NULL;
SemaphoreHandle_t sd_sempahore = NULL;
SemaphoreHandle_t cam_semaphore = NULL;

//TODO: make sure to delete the next first image if space isnt enough or the first image doesnt exists due to some random deleteing by user
#define BUFFER_MAX_SIZE 50000

#define PASSWORD "12345678"
bool streamKill;
int8_t streamCount = 0;          // Number of currently active streams
unsigned long streamsServed = 0; // Total completed streams
unsigned long imagesServed = 0;  // Total image requestss

void recordingTask(void* data)
{
    // first image is usually bad quality at startup so its discarded
    if (xSemaphoreTake(cam_semaphore, portMAX_DELAY))
    {
        camera_fb_t *fb =  esp_camera_fb_get();
        ESP_LOGI("Recording Task", "%d %d %d", fb->height, fb->width, fb->len);
        esp_camera_fb_return(fb);
        xSemaphoreGive(cam_semaphore);
    }
    uint32_t first_image = 1;
    uint32_t last_image = 0;

    if (exists("first.data"))
    {
        readBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);
       
    }
    else
    {
        writeBytes(MOUNT_POINT"/first.data", (void*)&first_image, sizeof(uint32_t), 1);

    }

    if (exists("last.data"))
    {
        readBytes(MOUNT_POINT"/last.data", (void*)&last_image, sizeof(uint32_t), 1);
        
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
        if (xSemaphoreTake(cam_semaphore, portMAX_DELAY)) 
        {
            if (xSemaphoreTake(sd_sempahore, portMAX_DELAY))
            {
                ESP_LOGI("Recording Task", "sd_semaphore taken succesfully");
                camera_fb_t* fb =  esp_camera_fb_get();
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
                xSemaphoreGive(cam_semaphore);
            }
            xSemaphoreGive(cam_semaphore);

        }
        last_image--;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);

}


static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    streamKill = false;

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        streamCount = 0;
        ESP_LOGE("HTTP_Server", "STREAM: failed to set HTTP response type");
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if(res == ESP_OK){
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }

    while(true) 
    {
        if (xSemaphoreTake(cam_semaphore, portMAX_DELAY))
        {
            
            fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE("HTTP_Server", "STREAM: failed to acquire frame");
                res = ESP_FAIL;
            } else {
                if(fb->format != PIXFORMAT_JPEG){
                    ESP_LOGE("HTTP_Server", "STREAM: Non-JPEG frame returned by camera module");
                    res = ESP_FAIL;
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
            if(res == ESP_OK){
                size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
                res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
            }
            if(res == ESP_OK){
                res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
            }
            if(res == ESP_OK){
                res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            }
            if(fb){
                esp_camera_fb_return(fb);
                xSemaphoreGive(cam_semaphore);
                fb = NULL;
                _jpg_buf = NULL;
            } else if(_jpg_buf){
                free(_jpg_buf);
                _jpg_buf = NULL;
            }
            if(res != ESP_OK){
                // This is the error exit point from the stream loop.
                // We end the stream here only if a Hard failure has been encountered or the connection has been interrupted.
                ESP_LOGE("HTTP_Server", "Stream failed, code = %i : %s\r\n", res, esp_err_to_name(res));
                break;
            }
            if(streamKill){
                // We end the stream here when a kill is signalled.
                ESP_LOGI("HTTP_Server","Stream killed\r\n");
                break;
            }
            int64_t frame_time = esp_timer_get_time() - last_frame;
            frame_time /= 1000;
            ESP_LOGI("HTTP_Server", "MJPG: %uB, framerate (%.1ffps)\r\n", (uint32_t)(_jpg_buf_len), 1000.0 / (uint32_t)(frame_time));

            last_frame = esp_timer_get_time();
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    streamsServed++;
    streamCount = 0;
    ESP_LOGI("HTTP_Server", "Stream ended");
    last_frame = 0;
    return res;
}


static esp_err_t stop_handler(httpd_req_t *req){
    streamKill = true;
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGI("stop_handler", "stopping stream");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t style_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)style_css, style_css_len);
}

static esp_err_t streamviewer_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)streamviewer_html, streamviewer_html_len);
}
static esp_err_t info_handler(httpd_req_t *req){
    static char json_response[256];
    char * p = json_response;
    char streamURL[50] = {0};
    int ip[4] = {192, 168, 0, 33};
    sprintf(streamURL, "http://%d.%d.%d.%d/stream", IP2STR(&sta_ip));
    *p++ = '{';
    p+=sprintf(p, "\"cam_name\":\"ESP32CAM-SERVER\",");
    p+=sprintf(p, "\"rotate\":\"%d\",", 0);
    p+=sprintf(p, "\"stream_url\":\"%s\"", streamURL);

    
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}


static esp_err_t index_handler(httpd_req_t *req){

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)index_simple_html, index_simple_html_len);
}

void app_main(void)
{

    camInit();
    sdInit();
    wifiInitSTA(STA_SSID, STA_PASSWORD);
    ntpInit("UTC-1");



    vSemaphoreCreateBinary(sd_sempahore);
    if(sd_sempahore == NULL)
    {
        ESP_LOGE("MAIN", "failed to create sd_sempahore");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    vSemaphoreCreateBinary(cam_semaphore);
    if(cam_semaphore == NULL)
    {
        ESP_LOGE("MAIN", "failed to create cam_semaphore");
        ESP_ERROR_CHECK(ESP_FAIL);
    }



    xTaskCreatePinnedToCore(recordingTask, "Recording Task", 8192*4, NULL, 5, &recording_task_handle, 0);
   
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16; // we use more than the default 8 (on port 80)

    httpd_uri_t stream_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t streamviewer_uri = {
        .uri       = "/view",
        .method    = HTTP_GET,
        .handler   = streamviewer_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t style_uri = {
        .uri       = "/style.css",
        .method    = HTTP_GET,
        .handler   = style_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stop_uri = {
        .uri       = "/stop",
        .method    = HTTP_GET,
        .handler   = stop_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t info_uri = {
        .uri       = "/info",
        .method    = HTTP_GET,
        .handler   = info_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&camera_httpd, &config) == ESP_OK) 
    {
 
        httpd_register_uri_handler(camera_httpd, &style_uri);
        httpd_register_uri_handler(camera_httpd, &stop_uri);
        httpd_register_uri_handler(camera_httpd, &stream_uri);
        httpd_register_uri_handler(camera_httpd, &info_uri);
        httpd_register_uri_handler(camera_httpd, &streamviewer_uri);

        
    }

    
        ESP_LOGI("MAIN", "Server strated");
    


}
