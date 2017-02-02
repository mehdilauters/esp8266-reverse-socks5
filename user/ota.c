#include "ota.h"
#include "config.h"
#include "main.h"

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "ssid_config.h"
// #include "mbedtls/sha256.h"

#include "ota-tftp.h"
#include "rboot-api.h"

/* Output of the command 'sha256sum firmware1.bin' */
static const char *FIRMWARE1_SHA256 = "";
#define TFTP_PORT 69

static char * m_ip;

static void tftpclient_download_and_verify(int slot, rboot_config *conf)
{
  char * filename = TFTP_IMAGE_FILENAME;
  printf("Downloading %s to slot %d...\n", filename, slot);
  int res = ota_tftp_download(m_ip, TFTP_PORT, filename, 1000, slot, NULL);
  printf("ota_tftp_download %s result %d\n", filename, res);
  
  if (res != 0) {
    return;
  }
  
  printf("Looks valid, calculating SHA256...\n");
  uint32_t length;
  bool valid = rboot_verify_image(conf->roms[slot], &length, NULL);
//   static mbedtls_sha256_context ctx;
//   mbedtls_sha256_init(&ctx);
//   mbedtls_sha256_starts(&ctx, 0);
//   valid = valid && rboot_digest_image(conf->roms[slot], length, (rboot_digest_update_fn)mbedtls_sha256_update, &ctx);
//   static uint8_t hash_result[32];
//   mbedtls_sha256_finish(&ctx, hash_result);
//   mbedtls_sha256_free(&ctx);
  
  if(!valid)
  {
    printf("Not valid after all :(\n");
    return;
  }
  
  printf("Image SHA256 = ");
  valid = true;
//   for(int i = 0; i < sizeof(hash_result); i++) {
//     char hexbuf[3];
//     snprintf(hexbuf, 3, "%02x", hash_result[i]);
//     printf(hexbuf);
//     if(strncmp(hexbuf, FIRMWARE1_SHA256+i*2, 2))
//       valid = false;
//   }
  printf("\n");
  
  if(!valid) {
    printf("Downloaded image SHA256 didn't match expected '%s'\n", FIRMWARE1_SHA256);
    return;
  }
  
  printf("SHA256 Matches. Rebooting into slot %d...\n", slot);
  rboot_set_current_rom(slot);
  sdk_system_restart();
}

void start_client() {
  printf("TFTP client task starting...\n");
  rboot_config conf;
  conf = rboot_get_config();
  int slot = (conf.current_rom + 1) % conf.count;
  printf("Image will be saved in OTA slot %d.\n", slot);
  if(slot == conf.current_rom) {
    printf("FATAL ERROR: Only one OTA slot is configured!\n");
    while(1) {}
  }
  
  
  tftpclient_download_and_verify(slot, &conf);
}

void otacheck_task(void *pvParameters) {
  while(1) {
    printf("=============%s\n",__TIME__);
    printf("Ota check");
          
    rboot_config conf = rboot_get_config();
    printf("\r\n\r\nOTA Basic demo.\r\nCurrently running on flash slot %d / %d.\r\n\r\n",
            conf.current_rom, conf.count);
    
    printf("Image addresses in flash:\r\n");
    for(int i = 0; i <conf.count; i++) {
      printf("%c%d: offset 0x%08x\r\n", i == conf.current_rom ? '*':' ', i, conf.roms[i]);
    }
    
    start_client();
    vTaskDelay(1000);
  }
}


void ota_start(const char * _ip) {
  m_ip = _ip;
  xTaskCreate(otacheck_task, (const char *)"otacheck_task", 512, NULL, 3, NULL);//1024,866
}
