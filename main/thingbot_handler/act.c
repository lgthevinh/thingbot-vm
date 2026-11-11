#include "act.h"
#include "esp_check.h"

static const char *TAG = "act";

esp_err_t act_init(act_handle_t *act, i2c_master_bus_handle_t bus) {
    ESP_RETURN_ON_ERROR(pca9685_create(bus, PCA9685_DEFAULT_INFO(), &act->pca), TAG, "pca9685 create failed");

    esp_err_t err = pca9685_config(act->pca, PCA9685_DEFAULT_CONFIG());
    if (err != ESP_OK) {
        // release the I2C device so init can be retried
        pca9685_delete(act->pca);
        ESP_LOGE(TAG, "pca9685 config failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t act_set_motor_speed(act_handle_t *act, int speed) {
    return ESP_OK;
}

esp_err_t act_set_servo_angle(act_handle_t *act, int angle) {
    return ESP_OK;
}

esp_err_t act_set_buzzer_pwm(act_handle_t *act, uint16_t pwm) {
    return ESP_OK;
}
