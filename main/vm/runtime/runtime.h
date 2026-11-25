#pragma once

typedef struct engine_t engine_t;

// Board environment that natives act on, defined by the board
typedef struct runtime_t runtime_t;

// Runtime function called by INVOKE_NATIVE, pops its arguments from the operand stack
typedef void (*NativeFn)(engine_t *e, runtime_t *rt);

// INVOKE_NATIVE ids are 16-bit: below the base runs on any runtime, from the base needs a specific runtime
#define NATIVE_SPECIFIC_BASE 0x8000
#define NATIVE_ID_MAX 0xFFFF

// Natives of one id range, fns[i] is id base + i
typedef struct {
    const NativeFn *fns;
    int count;
} native_table_t;
