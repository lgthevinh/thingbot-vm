#pragma once

#include "act.h"
#include "engine.h"
#include "io.h"

// ThingBot runtime, the board environment natives act on
struct runtime_t {
    io_t io;
    act_handle_t act;
};
