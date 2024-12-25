

#define LDR1_ADC_GPIO 33
#define LDR2_ADC_GPIO 32

#define ADC1_CHAN1 ADC_CHANNEL_5
#define ADC2_CHAN2 ADC_CHANNEL_4
#define ADC_WIDTH ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12

#define LDR1_ACTIVE_GPIO 23
#define LDR2_ACTIVE_GPIO 22

#define LDR_RANGE_MIN 150

void ldr_init();
int ldr1_get_value();
int ldr2_get_value();
void active_ldr1_sensor();
void active_ldr2_sensor();
void disable_ldr1_sensor();
void disable_ldr2_sensor();
void ldr_sun_detect(int *angle);