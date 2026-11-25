#include "vm.h"
#include "esp_log.h"
#include "handler.h"

static const char *TAG = "vm";

static void vm_exec(vm_t *vm, int opcode);

void vm_init(vm_t *vm, void *ctx, const NativeFn *natives, int native_count) {
    vm->ctx = ctx;
    vm->natives = natives;
    vm->native_count = native_count;
}

void vm_run(vm_t *vm, const int *prog) {
    vm->prog = prog;
    vm->pc = 0;  // program_counter
    vm->sp = -1; // stack_pointer
    vm->call_sp = -1;
    vm->running = true;

    while (vm->running) {
        int op_code = vm_fetch(vm);
        vm_exec(vm, op_code);
    }
}

static void vm_exec(vm_t *vm, int opcode) {
    if (opcode < 0 || opcode >= OP_COUNT || op_handlers[opcode] == NULL) {
        ESP_LOGE(TAG, "bad opcode %d, pc %d", opcode, vm->pc - 1);
        vm->running = false;
        return;
    }

    op_handlers[opcode](vm);
}
