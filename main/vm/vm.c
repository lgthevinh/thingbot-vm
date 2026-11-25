#include "vm.h"

void vm_init(vm_t *vm, runtime_t *runtime, const native_table_t *common, const native_table_t *specific) {
    vm->runtime = runtime;
    vm->common = common;
    vm->specific = specific;
}

void vm_load(vm_t *vm, const int *prog) {
    vm->prog = prog;
    engine_reset(&vm->engine);
}

EngineStatus vm_run(vm_t *vm) {
    // the caller has already paused for the last WAIT
    if (vm->engine.status == ENGINE_WAITING) {
        vm->engine.status = ENGINE_RUNNING;
    }

    while (vm->engine.status == ENGINE_RUNNING) {
        engine_step(&vm->engine, vm->prog, vm->common, vm->specific, vm->runtime);
    }

    return vm->engine.status;
}
