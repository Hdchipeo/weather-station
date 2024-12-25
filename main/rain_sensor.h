
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define RAIN_SENSOR_GPIO 16
#define RAIN_ACTIVE_GPIO 17
#define RAIN_ADC_GPIO 34

#define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_6
#define ADC_WIDTH ADC_BITWIDTH_12
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_12
#define TRAFFIX_MAX 3150

void rain_sensor_init();
bool rain_sensor_get_state();
int rain_get_traffic();
void active_rain_sensor();
void disable_rain_sensor();
