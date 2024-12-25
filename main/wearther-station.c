#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature_sensor.h"
#include "rain_sensor.h"
#include "espnow_station.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "battery.h"
#include "servo.h"
#include "ldr.h"

static const char *TAG_STATION = "WEATHER-STATION";

RTC_DATA_ATTR int angle = 90;

void weather_station_task()
{

    data_station_t station_data;

    while (1)
    {

        station_data.temperature = get_temperature_value();
        station_data.rain_state = rain_sensor_get_state();
        station_data.rain_traffic = rain_get_traffic();
        station_data.battery_capacity = battery_get_capacity();
        station_data.angle = angle;

        ESP_LOGI(TAG_STATION, "Temperature = %.1f°C", station_data.temperature);
        ESP_LOGI(TAG_STATION, "Rain state = %d", !station_data.rain_state);
        ESP_LOGI(TAG_STATION, "Rain traffic = %d mV", station_data.rain_traffic);
        ESP_LOGI(TAG_STATION, "Battery capacity = %.1f%%", station_data.battery_capacity);
        ESP_LOGI(TAG_STATION, "Servo angle = %d", angle);

        if (send_data(station_data) != ESP_OK)
        {
            ESP_LOGE(TAG_STATION, "Send data fail");
        }
        else
        {
            ESP_LOGI(TAG_STATION, "Send done");
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        disable_rain_sensor();
        disable_temp_sensor();

        ldr_sun_detect(&angle);

        enter_deep_sleep();
    }
}

void app_main(void)
{
    temperature_sensor_init();
    servo_init(angle);
    ldr_init();
    rain_sensor_init();
    battery_init();

    espnow_station_task_start();

    xTaskCreate(weather_station_task, "weather-station-task", configMINIMAL_STACK_SIZE * 3, NULL, 10, NULL);
}
