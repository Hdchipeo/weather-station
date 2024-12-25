
#include <stdio.h>
#include "driver/gpio.h"
#include "rain_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);
static void config_gpio_output(gpio_num_t gpioNum);

extern adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc2_cali_chan2_handle = NULL;

static const char *TAG = "RAIN_SENSOR";

void rain_sensor_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RAIN_SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    config_gpio_output(RAIN_ACTIVE_GPIO);

    active_rain_sensor();

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));

    bool do_calibration1_chan1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN1, EXAMPLE_ADC_ATTEN, &adc2_cali_chan2_handle);
}

bool rain_sensor_get_state()
{
    bool rain_state = 0;

    rain_state = gpio_get_level(RAIN_SENSOR_GPIO);

    return rain_state;
}

int rain_get_traffic()
{
    int adc_raw = 0;
    int voltage = 0;
    int actual_traffic = 0;

    adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw);
    adc_cali_raw_to_voltage(adc2_cali_chan2_handle, adc_raw, &voltage);

    ESP_LOGI(TAG, "ADC rain traffic raw = %d mV", voltage);

    actual_traffic = TRAFFIX_MAX - voltage;

    if (actual_traffic < 100)
        actual_traffic = 0;

    return actual_traffic;
}

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if CONFIG_IDF_TARGET_ESP32C3
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_WIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#elif CONFIG_IDF_TARGET_ESP32

    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_WIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if CONFIG_IDF_TARGET_ESP32C3
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif CONFIG_IDF_TARGET_ESP32

    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

static void config_gpio_output(gpio_num_t gpioNum)
{
    gpio_reset_pin(gpioNum);
    gpio_set_direction(gpioNum, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpioNum, GPIO_PULLUP_ONLY);
}

void active_rain_sensor()
{
    gpio_set_level(RAIN_ACTIVE_GPIO, 1);
}

void disable_rain_sensor()
{
    gpio_set_level(RAIN_ACTIVE_GPIO, 0);
}
