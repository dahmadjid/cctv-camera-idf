#ifndef _SD_UTILS_H_
#define _SD_UTILS_H_

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "driver/sdmmc_host.h"

#define SD_TAG "SDCARD"

#ifdef __cplusplus
extern "C" {
#endif


#define MOUNT_POINT "/sdcard"



/**
 * @brief Initializes SDMMC and mount the FAT file system
 * 
 */
void sdInit(void);

/**
 * @brief Simply write buffer into a file, used to write jpeg images from the framebuffer ptr
 * 
 * @param file_path name of the file to write to. example /mountpoint/img.jpg 
 * @param img ptr to buffer that holds image data
 * @param len number of bytes in the image
 */
void writeImg(const char* file_path, uint8_t* img, int len);

/**
 * @brief 
 * 
 * @param file_path name of the file to write to. example /mountpoint/newfile.txt 
 * @param text string you want to write
 */
void writeTxt(const char* file_path, const char* text);

/**
 * @brief 
 * 
 * @param file_path name of the file to read. example /mountpoint/newfile.txt 
 * @param buf output char buffer, currently supports reading upto 100 char including null terminator. enough for the purposes of the cctv-cam-project
 */
void readTxt(const char* file_path, char* buf);

#ifdef __cplusplus
}
#endif

#endif
