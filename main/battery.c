#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "battery.h"

const static char *TAG = "BATTERY_STATION";

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

extern adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_chan0_handle = NULL;

void battery_init()
{

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

    //-------------ADC1 Calibration Init---------------//

    example_adc_calibration_init(ADC_UNIT_1, ADC1_CHAN0, ADC_ATTEN, &adc1_cali_chan0_handle);
}

float battery_get_capacity()
{
    int adc_raw[100];
    int adc_sum = 0;
    int adc_medium = 0;
    int mili_voltage = 0.0;
    float voltage = 0;
    float min_voltage = 3.0;
    float max_voltage = 4.2;

    for (int i = 0; i < SAMPLE_BATTERY; i++)
    {
        adc_oneshot_read(adc1_handle, ADC1_CHAN0, &adc_raw[i]);
        adc_sum += adc_raw[i];
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    adc_medium = adc_sum / SAMPLE_BATTERY;

    // ESP_LOGI(TAG, "ADC medium = %d", adc_medium);

    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_medium, &mili_voltage);

    voltage = (float)mili_voltage / 1000;

    // ESP_LOGI(TAG, "ADC voltage = %.1f V", voltage);

    float actual_voltage = voltage * 2.0;

    // ESP_LOGI(TAG, "Battery actual voltage = %.1f V", actual_voltage);

    float percentage = (float)((actual_voltage - min_voltage) / (max_voltage - min_voltage)) * 100;

    if (percentage > 100.0)
        percentage = 100.0;
    if (percentage < 0.0)
        percentage = 0.0;

    // ESP_LOGI(TAG, "Battery capacity = %.1f%%", percentage);

    return percentage;
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