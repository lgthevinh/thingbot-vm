#include "opcode.h"
#include "thingbot.h"
#include "vm.h"

#define I2C_SCL_GPIO GPIO_NUM_9
#define I2C_SDA_GPIO GPIO_NUM_8

static thingbot_t bot;
static vm_t vm;

void app_main(void) {
    // clang-format off
    const int prog[] = {
        PSH, 10,   // x = 10
        STORE, 0,  // local[0] = x
        LOAD, 0,   // copy x from local[0] to stack
        PSH, 1000,
        GT,        // compare a (x) with b (1000)
        JNZ, 20,   // if not zero, jump to halt
        LOAD, 0,   // GT consume x, so get x again
        PSH, 5,
        MUL,       // x * 5
        STORE, 0,  // pop the value of stack into local[0]
        JMP, 4,    // jump to prog[4]
        HLT,
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
