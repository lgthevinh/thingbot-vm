#pragma once

#include "pca9685.h"
#include <stdint.h>

typedef struct {
    pca9685_handle_t pca;
} act_handle_t;

esp_err_t act_init(act_handle_t *act, i2c_master_bus_handle_t bus);
esp_err_t act_set_motor_speed(act_handle_t *act, int speed);
esp_err_t act_set_servo_angle(act_handle_t *act, int angle);
esp_err_t act_set_buzzer_pwm(act_handle_t *act, uint16_t pwm);
