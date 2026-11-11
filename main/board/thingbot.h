#pragma once

#include "act.h"
#include "io.h"

// Board context passed to the VM through vm_t.ctx
typedef struct {
    io_t io;
    act_handle_t act;
} thingbot_t;
