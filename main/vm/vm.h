#pragma once

#include "engine.h"

// Runs a loaded program on the engine against a runtime
typedef struct {
    const int *prog;         // program being run
    const NativeFn *natives; // runtime functions, indexed by INVOKE_NATIVE id
    int native_count;        // number of entries in natives
    engine_t engine;
    runtime_t *runtime;      // board environment, passed to every native
} vm_t;

void vm_init(vm_t *vm, runtime_t *runtime, const NativeFn *natives, int native_count);
void vm_load(vm_t *vm, const int *prog);
EngineStatus vm_run(vm_t *vm);
