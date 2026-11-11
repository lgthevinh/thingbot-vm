#include "io.h"
#include "esp_check.h"
#include <string.h>

static const char *TAG = "io";

void io_init(io_t *io) {
    memset(io, 0, sizeof(*io));
}

void io_set_i2c(io_t *io, i2c_master_bus_handle_t bus) {
    io->i2c_bus = bus;
}

void io_set_spi(io_t *io, spi_host_device_t host) {
    io->spi_host = host;
    io->spi_ready = true;
}

esp_err_t io_init_gpio(io_t *io, const gpio_config_t *in_cfg, const gpio_config_t *out_cfg) {
    if (in_cfg != NULL) {
        ESP_RETURN_ON_ERROR(gpio_config(in_cfg), TAG, "gpio input config failed");
        io->gpio_in_mask = in_cfg->pin_bit_mask;
    }
    if (out_cfg != NULL) {
        ESP_RETURN_ON_ERROR(gpio_config(out_cfg), TAG, "gpio output config failed");
        io->gpio_out_mask = out_cfg->pin_bit_mask;
    }
    return ESP_OK;
}

bool io_i2c_available(const io_t *io) {
    return io->i2c_bus != NULL;
}

bool io_spi_available(const io_t *io) {
    return io->spi_ready;
}

bool io_gpio_is_input(const io_t *io, int pin) {
    return GPIO_IS_VALID_GPIO(pin) && (io->gpio_in_mask & (1ULL << pin));
}

bool io_gpio_is_output(const io_t *io, int pin) {
    return GPIO_IS_VALID_GPIO(pin) && (io->gpio_out_mask & (1ULL << pin));
}
