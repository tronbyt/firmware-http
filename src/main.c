#include <assets.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <webp/demux.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

#include "display.h"
#include "flash.h"
#include "gfx.h"
#include "remote.h"
#include "sdkconfig.h"
#include "wifi.h"

#define BLUE "\033[1;34m"
#define RESET "\033[0m"  // Reset to default color

static const char* TAG = "main";
int32_t isAnimating =
    5;  // Initialize with a valid value enough time for boot animation
int32_t app_dwell_secs = TIDBYT_REFRESH_INTERVAL_SECONDS;

void memory_monitor_task(void* pvParameters) {
  while (1) {
    // Store the values first
    size_t free_heap = esp_get_free_heap_size();
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Then log them
    ESP_LOGI(TAG, "Free heap: %d, largest block: %d", free_heap, largest_block);

    // If largest block is too small, try to defragment
    if (largest_block < 10000) {
      ESP_LOGW(TAG, "Memory fragmented - largest block too small, forcing GC");
      // Force a light garbage collection by allocating and freeing memory
      void* temp = malloc(1024);
      if (temp) free(temp);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "App Main Start");

  // Setup the device flash storage.
  if (flash_initialize()) {
    ESP_LOGE(TAG, "failed to initialize flash");
    return;
  }
  esp_register_shutdown_handler(&flash_shutdown);

  // Create memory monitoring task
  xTaskCreate(memory_monitor_task, "mem_monitor", 2048, NULL, 1, NULL);

  // Setup the display.
  if (gfx_initialize(ASSET_BOOT_WEBP, ASSET_BOOT_WEBP_LEN)) {
    ESP_LOGE(TAG, "failed to initialize gfx");
    return;
  }
  esp_register_shutdown_handler(&display_shutdown);

  // Setup WiFi.
  if (wifi_initialize(TIDBYT_WIFI_SSID, TIDBYT_WIFI_PASSWORD)) {
    ESP_LOGE(TAG, "failed to initialize WiFi");
    return;
  }
  esp_register_shutdown_handler(&wifi_shutdown);

  uint8_t mac[6];
  if (!wifi_get_mac(mac)) {
    ESP_LOGI(TAG, "WiFi MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
             mac[2], mac[3], mac[4], mac[5]);
  }

  ESP_LOGW(TAG, "Main Loop Start");
  for (;;) {
    uint8_t* webp;
    size_t len;
    static int32_t brightness = DISPLAY_DEFAULT_BRIGHTNESS;

    if (remote_get(TIDBYT_REMOTE_URL, &webp, &len, &brightness,
                   &app_dwell_secs)) {
      ESP_LOGE(TAG, "Failed to get webp");
      vTaskDelay(pdMS_TO_TICKS(1 * 1000));
    } else {
      // Successful remote_get
      if (brightness > -1 && brightness < 256) {
        ESP_LOGI(TAG, BLUE "setting brightness to %d" RESET, (int)brightness);
        display_set_brightness(brightness);
      }
      ESP_LOGI(TAG, BLUE "Queuing new webp (%d bytes)" RESET, len);
      if (gfx_update(webp, len) == 1) {
        continue;
      } 
      free(webp);
      // Wait for app_dwell_secs to expire (isAnimating will be 0)
      if (isAnimating > 0) ESP_LOGI(TAG, BLUE "Delay for current webp" RESET);
      // More efficient polling with longer delay
      while (isAnimating > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms instead of 1ms
      }
      ESP_LOGI(TAG, BLUE "Showing new webp" RESET);
      isAnimating = app_dwell_secs;  // use isAnimating as the container for
                                     // app_dwell_secs
    }
  }
}