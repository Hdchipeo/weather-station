#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "driver/gpio.h"
#include "espnow_station.h"
#include "esp_err.h"
#include "esp_sleep.h"

#define ESPNOW_MAXDELAY 512

static const char *TAG = "espnow_weather_station";

static QueueHandle_t s_example_espnow_queue;

// static uint8_t peer_addr[6] = {0xe4, 0x65, 0xb8, 0x11, 0x7f, 0x9c}; // Pc hub address for esp32

static uint8_t peer_addr[6] = {0x48, 0xca, 0x43, 0xcf, 0xff, 0x68}; // Pc hub address for esp32c3

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));

    int8_t tx_power = 80;
    esp_wifi_set_max_tx_power(tx_power);
    // esp_wifi_get_max_tx_power(&tx_power);
    // ESP_LOGI(TAG, "Tx power value = %d", tx_power);
}

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    example_espnow_event_t evt;
    example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send send queue fail");
    }
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t *mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

static void example_espnow_task(void *pvParameter)
{
    example_espnow_event_t evt;

    while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE)
    {

        switch (evt.id)
        {
        case EXAMPLE_ESPNOW_SEND_CB:
        {
            break;
        }
        case EXAMPLE_ESPNOW_RECV_CB:
        {
            break;
        }
        default:

            break;
        }
    }
}

static esp_err_t example_espnow_init(void)
{

    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));

    free(peer);

    xTaskCreate(example_espnow_task, "example_espnow_task", 2048, NULL, 4, NULL);

    return ESP_OK;
}

static void example_espnow_deinit()
{
    // free(send_param->buffer);
    // free(send_param);
    // vSemaphoreDelete(s_example_espnow_queue);
    esp_now_deinit();
}

void espnow_station_task_start()
{

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    example_wifi_init();

    // Set wake interval for ESP-NOW
    set_wake_interval();

    // Enable power save mode
    enable_power_save();

    example_espnow_init();

    // Set wake window for ESP-NOW
    set_wake_window();
}

esp_err_t send_data(data_station_t station_data)
{
    size_t len = sizeof(data_station_t);
    uint8_t *data = (uint8_t *)malloc(len);
    if (data == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    memcpy(data, &station_data, len);

    esp_err_t check;
    check = esp_now_send(peer_addr, data, len);

    free(data);

    return check;
}
void set_wake_window()
{
    uint16_t wake_window_us = 1000; // 1ms
    esp_err_t err = esp_now_set_wake_window(wake_window_us);
    if (err == ESP_OK)
    {
        printf("Wake window set to %d us\n", wake_window_us);
    }
    else
    {
        printf("Failed to set wake window: %s\n", esp_err_to_name(err));
    }
}
void set_wake_interval()
{
    uint16_t wake_interval_ms = 100; // 100 ms
    esp_err_t err = esp_wifi_connectionless_module_set_wake_interval(wake_interval_ms);
    if (err == ESP_OK)
    {
        printf("Wake interval set to %d ms\n", wake_interval_ms);
    }
    else
    {
        printf("Failed to set wake interval: %s\n", esp_err_to_name(err));
    }
}

void enable_power_save()
{
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if (err == ESP_OK)
    {
        printf("Power save mode enabled\n");
    }
    else
    {
        printf("Failed to enable power save mode: %s\n", esp_err_to_name(err));
    }
}

void enter_light_sleep()
{

    // Set timer wakeup
    esp_sleep_enable_timer_wakeup(30 * 1000000); // 30 second

    example_espnow_deinit();
    esp_wifi_stop();

    esp_light_sleep_start();
}

void enter_deep_sleep()
{
    // Set timer wakeup
    esp_sleep_enable_timer_wakeup(30 * 1000000); // 30 second

    example_espnow_deinit();
    esp_wifi_stop();

    esp_deep_sleep_start();
}

void station_wakeup()
{
    ESP_ERROR_CHECK(esp_wifi_start());
    example_espnow_init();
}