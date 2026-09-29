#include "wifi.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "string.h"

#define CONNECTED_BIT BIT0

static const char *TAG = "wifi";
static EventGroupHandle_t events;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();

  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_event_sta_disconnected_t *e = data;
    xEventGroupClearBits(events, CONNECTED_BIT);
    ESP_LOGW(TAG, "Unconnected (reason %d), reconnecting...", e->reason);
    esp_wifi_connect();

  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *e = data;
    ESP_LOGI(TAG, "Connected, IP: " IPSTR, IP2STR(&e->ip_info.ip));
    xEventGroupSetBits(events, CONNECTED_BIT);
  }
}

static esp_err_t init_nvs(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Unable to erase NVS");
    err = nvs_flash_init();
  }
  return err;
}

esp_err_t wifi_start(const char *ssid, const char *password) {
  events = xEventGroupCreate();

  ESP_RETURN_ON_ERROR(init_nvs(), TAG, "NVS");
  ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif");
  ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop");
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_RETURN_ON_ERROR(esp_wifi_init(&init_cfg), TAG, "wifi init");

  ESP_RETURN_ON_ERROR(
      esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL),
      TAG, "handler");
  ESP_RETURN_ON_ERROR(
      esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL),
      TAG, "handler");

  wifi_config_t cfg = {
      .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
  };

  strlcpy((char *)cfg.sta.ssid, ssid, sizeof cfg.sta.ssid);
  strlcpy((char *)cfg.sta.password, password, sizeof cfg.sta.password);

  ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
  ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &cfg), TAG, "config");
  ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");

  return ESP_OK;
}

bool wifi_wait_connected(TickType_t timeout) {
  EventBits_t bits =
      xEventGroupWaitBits(events, CONNECTED_BIT, pdFALSE, pdTRUE, timeout);
  return bits & CONNECTED_BIT;
}
