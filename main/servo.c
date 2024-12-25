
#include "esp_log.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "servo.h"
#include "esp_err.h"
#include "iot_servo.h"
#include "driver/mcpwm_prelude.h"

static const char *TAG = "SERVO";
mcpwm_cmpr_handle_t comparator = NULL;
int current_servo_angle = 0;

static inline uint32_t angle_to_compare(int angle)
{
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

static void config_gpio_output(gpio_num_t gpioNum)
{
    gpio_reset_pin(gpioNum);
    gpio_set_direction(gpioNum, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpioNum, GPIO_PULLUP_ONLY);
}
void active_servo()
{
}
void disable_servo()
{
}
void servo_init(int angle_current)
{
    // config_gpio_output(SERVO_POWER_ACTIVE);
    // active_servo();

    // servo_config_t servo_cfg = {
    //     .max_angle = 180,
    //     .min_width_us = 500,
    //     .max_width_us = 2500,
    //     .freq = 50,
    //     .timer_number = LEDC_TIMER_0,
    //     .channels = {
    //         .servo_pin = {
    //             SERVO_PIN,
    //         },
    //         .ch = {
    //             LEDC_CHANNEL_0,
    //         },
    //     },
    //     .channel_number = 1,
    // };
    // iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);

    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = SERVO_PULSE_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(angle_current)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

void set_servo_angle(int angle)
{
    // iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, angle);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(angle)));
    current_servo_angle = angle;
}

static int get_current_servo_angle()
{
    return current_servo_angle;
}

void move_servo_smooth(int angle)
{
    int current_angle = get_current_servo_angle();
    int distance = abs(angle - current_angle);
    int accel_steps = distance / 3;
    int constant_steps = distance - 2 * accel_steps;

    int step = (current_angle < angle) ? 1 : -1;
    int delay_ms;

    // Speed up
    for (int i = 1; i <= accel_steps; i++)
    {
        current_angle += step;
        set_servo_angle(current_angle);
        delay_ms = 10 + (accel_steps - i);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    // Run evenly
    for (int i = 0; i < constant_steps; i++)
    {
        current_angle += step;
        set_servo_angle(current_angle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Slow down
    for (int i = accel_steps; i > 0; i--)
    {
        current_angle += step;
        set_servo_angle(current_angle);
        delay_ms = 10 + (accel_steps - i);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
