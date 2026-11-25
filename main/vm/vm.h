#pragma once

#include "engine.h"

// Runs a loaded program on the engine against a runtime
typedef struct {
    const int *prog;                // program being run
    const native_table_t *common;   // natives every runtime provides, ids below NATIVE_SPECIFIC_BASE
    const native_table_t *specific; // natives of this runtime only, ids from NATIVE_SPECIFIC_BASE
    engine_t engine;
    runtime_t *runtime;             // board environment, passed to every native
} vm_t;

void vm_init(vm_t *vm, runtime_t *runtime, const native_table_t *common, const native_table_t *specific);
void vm_load(vm_t *vm, const int *prog);
EngineStatus vm_run(vm_t *vm);
