#pragma once

#include "vm.h"

// INVOKE_NATIVE ids on ThingBot
typedef enum {
    MOTOR,
    SERVO,
    SPIN,
    BUZZ,
    LED,

    NATIVE_COUNT,
} NativeId;

extern const NativeFn thingbot_natives[NATIVE_COUNT];
