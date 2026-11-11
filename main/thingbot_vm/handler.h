#pragma once

#include "opcode.h"
#include "vm.h"

typedef void (*OpHandler)(vm_t *vm);

extern const OpHandler op_handlers[OP_COUNT];
