#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    i2c_master_bus_handle_t i2c_bus; // NULL when not attached
    spi_host_device_t spi_host;
    bool spi_ready;
    uint64_t gpio_in_mask;
    uint64_t gpio_out_mask;
} io_t;

void io_init(io_t *io);
void io_set_i2c(io_t *io, i2c_master_bus_handle_t bus);
void io_set_spi(io_t *io, spi_host_device_t host);
esp_err_t io_init_gpio(io_t *io, const gpio_config_t *in_cfg, const gpio_config_t *out_cfg);

bool io_i2c_available(const io_t *io);
bool io_spi_available(const io_t *io);
bool io_gpio_is_input(const io_t *io, int pin);
bool io_gpio_is_output(const io_t *io, int pin);
