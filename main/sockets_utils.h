#ifndef _SOCKETS_UTILS_H_
#define _SOCKETS_UTILS_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"



#define SOCK_TAG "SOCKETS"

#ifdef __cplusplus
extern "C" {
#endif




void serverInit();



#ifdef __cplusplus
}
#endif

#endif
