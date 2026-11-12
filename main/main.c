#include "opcode.h"
#include "thingbot.h"
#include "vm.h"

#define I2C_SCL_GPIO GPIO_NUM_9
#define I2C_SDA_GPIO GPIO_NUM_8

static thingbot_t bot;
static vm_t vm;

void app_main(void) {
    // clang-format off
    // beep and blink LED 1 three times
    const int prog[] = {
        PSH, 3,    // 0:  count = 3
        STORE, 0,  // 2:  local[0] = count
        PSH, 1,    // 4:  loop: LED 1 on
        PSH, 100,
        LED,
        PSH, 50,   // 9:  buzzer on, 50% duty
        BUZZ,
        PSH, 200,  // 12: wait 200 ms
        WAIT,
        PSH, 1,    // 15: LED 1 off
        PSH, 0,
        LED,
        PSH, 0,    // 20: buzzer off, outputs hold their last value after HLT
        BUZZ,
        PSH, 200,  // 23: wait 200 ms
        WAIT,
        LOAD, 0,   // 26: count - 1
        PSH, 1,
        SUB,
        STORE, 0,
        LOAD, 0,   // 33: loop while count != 0
        JNZ, 4,
        HLT,       // 37
    };
    // clang-format on

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0, // the C3 has a single I2C port
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    i2c_master_bus_handle_t i2c_bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    io_init(&bot.io);
    io_set_i2c(&bot.io, i2c_bus);
    ESP_ERROR_CHECK(act_init(&bot.act, bot.io.i2c_bus));

    vm_init(&vm, &bot);
    vm_run(&vm, prog);
}
