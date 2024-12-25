#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "ldr.h"
#include "rain_sensor.h"
#include "servo.h"

static const char *TAG = "LDR";

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_chan1_handle;

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

int ldr1_get_value()
{
    int value_raw = 0;
    int actual_value = 0;

    adc_oneshot_read(adc1_handle, ADC1_CHAN1, &value_raw);
    adc_cali_raw_to_voltage(adc1_cali_chan1_handle, value_raw, &actual_value);

    return actual_value;
}

int ldr2_get_value()
{
    int value_raw = 0;
    int actual_value = 0;

    adc_oneshot_read(adc1_handle, ADC2_CHAN2, &value_raw);
    adc_cali_raw_to_voltage(adc1_cali_chan1_handle, value_raw, &actual_value);

    return actual_value;
}

static void config_gpio_output(gpio_num_t gpioNum)
{
    gpio_reset_pin(gpioNum);
    gpio_set_direction(gpioNum, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpioNum, GPIO_PULLUP_ONLY);
}
void active_ldr1_sensor()
{
    gpio_set_level(LDR1_ACTIVE_GPIO, 1);
}
void active_ldr2_sensor()
{
    gpio_set_level(LDR2_ACTIVE_GPIO, 1);
}
void disable_ldr1_sensor()
{
    gpio_set_level(LDR1_ACTIVE_GPIO, 0);
}
void disable_ldr2_sensor()
{
    gpio_set_level(LDR2_ACTIVE_GPIO, 0);
}
void ldr_init()
{
    config_gpio_output(LDR1_ACTIVE_GPIO);
    config_gpio_output(LDR2_ACTIVE_GPIO);
    active_ldr1_sensor();
    active_ldr2_sensor();

    //-------------ADC1 Init---------------//

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN1, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC2_CHAN2, &config));

    example_adc_calibration_init(ADC_UNIT_1, ADC1_CHAN1, ADC_ATTEN, &adc1_cali_chan1_handle);
    example_adc_calibration_init(ADC_UNIT_1, ADC2_CHAN2, ADC_ATTEN, &adc1_cali_chan1_handle);
}

void ldr_sun_detect(int *angle)
{
    int ldr1_value = 0;
    int ldr2_value = 0;

    ldr1_value = ldr1_get_value();
    ldr2_value = ldr2_get_value();

    // ESP_LOGI(TAG, " Ldr1 value = %d", ldr1_value);
    // ESP_LOGI(TAG, " Ldr2 value = %d", ldr2_value);

    vTaskDelay(pdMS_TO_TICKS(10));
    disable_ldr1_sensor();
    disable_ldr2_sensor();

    if (ldr1_value > ldr2_value)
    {
        if (ldr1_value - ldr2_value > LDR_RANGE_MIN)
        {
            *angle += SERVO_STEP;
            if (*angle >= ANGLE_MAX)
                *angle = ANGLE_MAX;
            else
            {
                set_servo_angle(*angle);
                // move_servo_smooth(*angle);
            }
        }
    }
    else
    {
        if (ldr2_value - ldr1_value > LDR_RANGE_MIN)
        {
            *angle -= SERVO_STEP;

            if (*angle <= ANGLE_MIN)
                *angle = ANGLE_MIN;
            else
            {
                set_servo_angle(*angle);
                // move_servo_smooth(*angle);
            }
        }
    }
}
