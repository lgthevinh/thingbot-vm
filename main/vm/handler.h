#pragma once

#include "engine.h"
#include "opcode.h"

typedef void (*OpHandler)(engine_t *e, const int *prog);

extern const OpHandler op_handlers[OP_COUNT];
