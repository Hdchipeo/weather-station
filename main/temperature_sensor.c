#include "ds18b20.h"
#include "onewire_bus.h"
#include "temperature_sensor.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

ds18b20_device_handle_t ds18b20s;

static const char *TAG = "Temperature-sensor";

static void config_gpio_output(gpio_num_t gpioNum);

void temperature_sensor_init()
{
    config_gpio_output(TEMP_ACTIVE_GPIO);

    active_temp_sensor();

    vTaskDelay(pdMS_TO_TICKS(10));

    // install 1-wire bus
    onewire_bus_handle_t bus = NULL;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_BUS_GPIO,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));

    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    // create 1-wire device iterator, which is used for device search
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "Device iterator created, start searching...");
    search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
    if (search_result == ESP_OK)
    { // found a new device, let's check if we can upgrade it to a DS18B20
        ds18b20_config_t ds_cfg = {};
        // check if the device is a DS18B20, if so, return the ds18b20 handle
        if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &ds18b20s) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found a Device, address: %016llX", next_onewire_device.address);
        }
        else
        {
            ESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
        }
    }
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    ESP_LOGI(TAG, "Searching done");
}

float get_temperature_value()
{
    float temperature = 0.0;
    ds18b20_trigger_temperature_conversion(ds18b20s);
    ds18b20_get_temperature(ds18b20s, &temperature);
    // ESP_LOGI(TAG, "temperature read from DS18B20: %.2fC", temperature);

    return temperature;
}

static void config_gpio_output(gpio_num_t gpioNum)
{
    gpio_reset_pin(gpioNum);
    gpio_set_direction(gpioNum, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpioNum, GPIO_PULLUP_ONLY);
}

void active_temp_sensor()
{
    gpio_set_level(TEMP_ACTIVE_GPIO, 1);
}

void disable_temp_sensor()
{
    gpio_set_level(TEMP_ACTIVE_GPIO, 0);
}