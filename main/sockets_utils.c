#include "sockets_utils.h"

#define PORT 6205

static int sock = -1;
struct sockaddr_in server_addr = {0};
TaskHandle_t server_task_handle = 0;



static void serverTask(void* client_handler)
{

    if (client_handler == NULL)
    {
        ESP_LOGE(SOCK_TAG, "Client Handler is null ptr, server stopped");
        vTaskDelete(NULL);
    }
    while (1)
    {
        struct sockaddr_in client_addr = {0};
        int * client_ptr = (int*)malloc(sizeof(int));
        size_t len = sizeof(client_addr);
        *client_ptr = accept(sock, (struct sockaddr*)&client_addr, &len);
        
        if (*client_ptr < 0)
        {
            ESP_LOGE(SOCK_TAG, "Server unable to accept: errno %s", strerror(errno));   
        }
        else
        {
            ESP_LOGI(SOCK_TAG, "Accepted client");
            
            int ret = xTaskCreatePinnedToCore(client_handler, "client handler task", 1024, (void*)client_ptr, 10, NULL, 1);
            if (ret != pdPASS)
            {
                ESP_LOGE(SOCK_TAG, "Failed to create client handler, freertos err = %d", ret);
                closesocket(*client_ptr);
            }
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    } 
}

void serverInit(void* client_handler)
{
    server_addr.sin_len = 0;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;


    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) 
    {
        ESP_LOGE(SOCK_TAG, "Unable to create socket: errno %s", strerror(errno));
        return;   
    }
    else
    {
        ESP_LOGI(SOCK_TAG, "Successfully created socket with channel: %d", sock);
    }   

    int err = bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err != 0) 
    {
        ESP_LOGE(SOCK_TAG, "Socket unable to bind: errno %s", strerror(errno));   
    }
    else
    {
        ESP_LOGI(SOCK_TAG, "Successfully bound");
    }

    err = listen(sock, 5);
    if (err != 0) 
    {
        ESP_LOGE(SOCK_TAG, "Socket unable to listen: errno %s", strerror(errno));   
    }
    else
    {
        ESP_LOGI(SOCK_TAG, "Successfully listening");
    }
    while (1)
    {
        struct sockaddr_in client_addr = {0};
        int * client_ptr = (int*)malloc(sizeof(int));
        size_t len = sizeof(client_addr);
        *client_ptr = accept(sock, (struct sockaddr*)&client_addr, &len);
        
        if (*client_ptr < 0)
        {
            ESP_LOGE(SOCK_TAG, "Server unable to accept: errno %s", strerror(errno));   
        }
        else
        {
            ESP_LOGI(SOCK_TAG, "Accepted client");
            
            int ret = xTaskCreatePinnedToCore(client_handler, "client handler task", 1024*2, (void*)client_ptr, 10, NULL, 1);
            if (ret != pdPASS)
            {
                ESP_LOGE(SOCK_TAG, "Failed to create client handler, freertos err = %d", ret);
                closesocket(*client_ptr);
                free(client_ptr);
            }
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    } 
}



