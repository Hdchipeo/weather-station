
#define BATTERY_ADC_GPIO 35

#define ADC1_CHAN0 ADC_CHANNEL_7
#define ADC_WIDTH ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12
#define SAMPLE_BATTERY 10

void battery_init();
float battery_get_capacity();
