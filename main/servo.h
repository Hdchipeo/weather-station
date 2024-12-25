
#define SERVO_MIN_PULSEWIDTH_US 500  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2400 // Maximum pulse width in microsecond
#define SERVO_MIN_DEGREE 0           // Minimum angle
#define SERVO_MAX_DEGREE 180         // Maximum angle

#define SERVO_PULSE_GPIO 21                  // GPIO connects to the PWM signal line
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 20000          // 20000 ticks, 20ms

#define ANGLE_MAX 150
#define ANGLE_MIN 30
#define SERVO_STEP 5

void servo_init(int angle_current);
void set_servo_angle(int angle);
void move_servo_smooth(int angle);
void active_servo();
void disable_servo();