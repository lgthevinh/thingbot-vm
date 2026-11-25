#pragma once

#include "act.h"
#include "io.h"
#include "runtime.h"

// ThingBot runtime, the board environment natives act on
struct runtime_t {
    io_t io;
    act_handle_t act;
};

// INVOKE_NATIVE ids specific to ThingBot
typedef enum {
    MOTOR = NATIVE_SPECIFIC_BASE,
    SERVO,
    SPIN,
    BUZZ,
    LED,

    NATIVE_END,
} NativeId;

extern const native_table_t thingbot_common;
extern const native_table_t thingbot_specific;
