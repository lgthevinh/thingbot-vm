#pragma once

#include "pca9685.h"
#include <stdint.h>

typedef struct {
    pca9685_handle_t pca;
} act_handle_t;

// Device ids are 1-based and match the board labels (M1, SERVO_1, LED_1).
// Values outside their range are clamped.
esp_err_t act_init(act_handle_t *act, i2c_master_bus_handle_t bus);
esp_err_t act_set_motor_speed(act_handle_t *act, int id, int speed);
esp_err_t act_set_servo_angle(act_handle_t *act, int id, int angle);
esp_err_t act_set_servo_speed(act_handle_t *act, int id, int speed);
esp_err_t act_set_buzzer(act_handle_t *act, int duty);
esp_err_t act_set_led(act_handle_t *act, int id, int brightness);
