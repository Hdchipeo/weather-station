
#define ONEWIRE_BUS_GPIO 14
#define TEMP_ACTIVE_GPIO 12

void temperature_sensor_init();
float get_temperature_value();
void active_temp_sensor();
void disable_temp_sensor();