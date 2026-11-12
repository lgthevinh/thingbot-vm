#include "act.h"
#include "esp_check.h"
#include <stdlib.h>

// PCA9685 channel assignments — DC motors
#define M1_A 2
#define M1_B 3
#define M2_A 4
#define M2_B 5
#define M3_A 6
#define M3_B 7
#define M4_A 1
#define M4_B 0

// PCA9685 channel assignments — Servos
#define SERVO_1 12
#define SERVO_2 11
#define SERVO_3 10
#define SERVO_4 9
#define SERVO_5 8

// PCA9685 channel assignments — Buzzer and LEDs
#define BUZZER 14
#define LED_1 15
#define LED_2 13

// one prescaler for all channels, 50 Hz is what servos need
#define PWM_FREQ_HZ 50
#define PWM_PERIOD_US (1000000 / PWM_FREQ_HZ)
#define PWM_TICKS 4096
#define PWM_MAX (PWM_TICKS - 1)

// 180 servo: 0..180 degrees maps to SERVO_MIN_US..SERVO_MAX_US
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500

// 360 servo: -100..100 maps to SPIN_STOP_US -/+ SPIN_RANGE_US, trim SPIN_STOP_US if it creeps at 0
#define SPIN_STOP_US 1500
#define SPIN_RANGE_US 500

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *TAG = "act";

// indexed by id - 1, motors use an A/B channel pair
static const uint8_t motor_ch[][2] = {{M1_A, M1_B}, {M2_A, M2_B}, {M3_A, M3_B}, {M4_A, M4_B}};
static const uint8_t servo_ch[] = {SERVO_1, SERVO_2, SERVO_3, SERVO_4, SERVO_5};
static const uint8_t led_ch[] = {LED_1, LED_2};

static int clamp(int val, int lo, int hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

static uint16_t percent_to_ticks(int percent) {
    return percent * PWM_MAX / 100;
}

static uint16_t us_to_ticks(int us) {
    return us * PWM_TICKS / PWM_PERIOD_US;
}

static esp_err_t servo_set_us(act_handle_t *act, int id, int us) {
    if (id < 1 || id > ARRAY_LEN(servo_ch)) {
        return ESP_ERR_INVALID_ARG;
    }
    return pca9685_set_pwm(act->pca, servo_ch[id - 1], us_to_ticks(us));
}

esp_err_t act_init(act_handle_t *act, i2c_master_bus_handle_t bus) {
    ESP_RETURN_ON_ERROR(pca9685_create(bus, PCA9685_DEFAULT_INFO(), &act->pca), TAG, "pca9685 create failed");

    pca9685_config_t config = {.frequency_hz = PWM_FREQ_HZ};
    esp_err_t err = pca9685_config(act->pca, config);
    if (err != ESP_OK) {
        // release the I2C device so init can be retried
        pca9685_delete(act->pca);
        ESP_LOGE(TAG, "pca9685 config failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t act_set_motor_speed(act_handle_t *act, int id, int speed) {
    if (id < 1 || id > ARRAY_LEN(motor_ch)) {
        return ESP_ERR_INVALID_ARG;
    }

    // drive A for forward, B for reverse, both low at 0 to coast
    speed = clamp(speed, -100, 100);
    uint16_t duty = percent_to_ticks(abs(speed));
    const uint8_t *ch = motor_ch[id - 1];

    ESP_RETURN_ON_ERROR(pca9685_set_pwm(act->pca, ch[0], speed > 0 ? duty : 0), TAG, "motor %d A failed", id);
    return pca9685_set_pwm(act->pca, ch[1], speed < 0 ? duty : 0);
}

esp_err_t act_set_servo_angle(act_handle_t *act, int id, int angle) {
    angle = clamp(angle, 0, 180);
    return servo_set_us(act, id, SERVO_MIN_US + angle * (SERVO_MAX_US - SERVO_MIN_US) / 180);
}

esp_err_t act_set_servo_speed(act_handle_t *act, int id, int speed) {
    speed = clamp(speed, -100, 100);
    return servo_set_us(act, id, SPIN_STOP_US + speed * SPIN_RANGE_US / 100);
}

esp_err_t act_set_buzzer(act_handle_t *act, int duty) {
    return pca9685_set_pwm(act->pca, BUZZER, percent_to_ticks(clamp(duty, 0, 100)));
}

esp_err_t act_set_led(act_handle_t *act, int id, int brightness) {
    if (id < 1 || id > ARRAY_LEN(led_ch)) {
        return ESP_ERR_INVALID_ARG;
    }
    return pca9685_set_pwm(act->pca, led_ch[id - 1], percent_to_ticks(clamp(brightness, 0, 100)));
}
