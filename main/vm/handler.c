#include "handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "vm";

static bool vm_check_local(vm_t *vm, int idx) {
    if (idx >= 0 && idx < VM_LOCAL_SLOT) {
        return true;
    }

    ESP_LOGE(TAG, "bad local idx %d, pc %d", idx, vm->pc - 2);
    vm->running = false;
    return false;
}

static void op_hlt(vm_t *vm) {
    vm->running = false;

    ESP_LOGI(TAG, "halt, pc %d", vm->pc - 1);
}

static void op_psh(vm_t *vm) {
    int val = vm_fetch(vm);
    vm_push(vm, val);

    ESP_LOGD(TAG, "psh %d, pc %d, sp %d", val, vm->pc - 2, vm->sp);
}

static void op_add(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a + b);

    ESP_LOGD(TAG, "add %d + %d = %d, pc %d, sp %d", a, b, a + b, vm->pc - 1, vm->sp);
}

static void op_sub(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a - b);

    ESP_LOGD(TAG, "sub %d - %d = %d, pc %d, sp %d", a, b, a - b, vm->pc - 1, vm->sp);
}

static void op_mul(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a * b);

    ESP_LOGD(TAG, "mul %d * %d = %d, pc %d, sp %d", a, b, a * b, vm->pc - 1, vm->sp);
}

static void op_load(vm_t *vm) {
    int idx = vm_fetch(vm);
    if (!vm_check_local(vm, idx)) {
        return;
    }
    vm_push(vm, vm->local[idx]);

    ESP_LOGD(TAG, "load [%d] = %d, pc %d, sp %d", idx, vm->local[idx], vm->pc - 2, vm->sp);
}

static void op_store(vm_t *vm) {
    int idx = vm_fetch(vm);
    if (!vm_check_local(vm, idx)) {
        return;
    }
    vm->local[idx] = vm_pop(vm);

    ESP_LOGD(TAG, "store [%d] = %d, pc %d, sp %d", idx, vm->local[idx], vm->pc - 2, vm->sp);
}

static void op_jmp(vm_t *vm) {
    int pc = vm_fetch(vm);

    ESP_LOGD(TAG, "jmp %d -> %d, sp %d", vm->pc - 2, pc, vm->sp);
    vm->pc = pc;
}

static void op_jnz(vm_t *vm) {
    int target_pc = vm_fetch(vm);

    // pop the condition when read
    // not to hold the condition in the stack, avoid stack overflow in loops
    int cond = vm_pop(vm);
    if (cond == 0) {
        ESP_LOGD(TAG, "jnz not taken, pc %d, sp %d", vm->pc - 2, vm->sp);
        return;
    }

    ESP_LOGD(TAG, "jnz %d -> %d, sp %d", vm->pc - 2, target_pc, vm->sp);
    vm->pc = target_pc;
}

static void op_gt(vm_t *vm) {
    // pop b and a from stack, then psh:
    // 1 if a is greater than b, else 0
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a > b);

    ESP_LOGD(TAG, "gt %d > %d = %d, pc %d, sp %d", a, b, a > b, vm->pc - 1, vm->sp);
}

static void op_gte(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a >= b);

    ESP_LOGD(TAG, "gte %d >= %d = %d, pc %d, sp %d", a, b, a >= b, vm->pc - 1, vm->sp);
}

static void op_lt(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a < b);

    ESP_LOGD(TAG, "lt %d < %d = %d, pc %d, sp %d", a, b, a < b, vm->pc - 1, vm->sp);
}

static void op_lte(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a <= b);

    ESP_LOGD(TAG, "lte %d <= %d = %d, pc %d, sp %d", a, b, a <= b, vm->pc - 1, vm->sp);
}

static void op_eq(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a == b);

    ESP_LOGD(TAG, "eq %d == %d = %d, pc %d, sp %d", a, b, a == b, vm->pc - 1, vm->sp);
}

static void op_neq(vm_t *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a != b);

    ESP_LOGD(TAG, "neq %d != %d = %d, pc %d, sp %d", a, b, a != b, vm->pc - 1, vm->sp);
}

static void op_call(vm_t *vm) {
    int target_pc = vm_fetch(vm);
    if (vm->call_sp >= VM_CALL_DEPTH - 1) {
        ESP_LOGE(TAG, "call stack overflow, pc %d", vm->pc - 2);
        vm->running = false;
        return;
    }

    // pc already points past the operand, which is the return address
    vm_callstack_push(vm, vm->pc);
    vm->pc = target_pc;
}

static void op_ret(vm_t *vm) {
    if (vm->call_sp < 0) {
        ESP_LOGE(TAG, "ret with empty call stack, pc %d", vm->pc - 1);
        vm->running = false;
        return;
    }

    vm->pc = vm_callstack_pop(vm);
}

static void op_wait(vm_t *vm) {
    int ms = vm_pop(vm);
    if (ms < 0) {
        ms = 0;
    }

    // block the task instead of spinning so the idle task and watchdog keep running
    ESP_LOGD(TAG, "wait %d ms, pc %d, sp %d", ms, vm->pc - 1, vm->sp);
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void op_invoke_static(vm_t *vm) {
    ESP_LOGE(TAG, "invoke static not implemented, pc %d", vm->pc - 1);
    vm->running = false;
}

static void op_invoke_native(vm_t *vm) {
    int id = vm_fetch(vm);
    if (id < 0 || id >= vm->native_count || vm->natives[id] == NULL) {
        ESP_LOGE(TAG, "bad native id %d, pc %d", id, vm->pc - 2);
        vm->running = false;
        return;
    }

    ESP_LOGD(TAG, "invoke native %d, pc %d, sp %d", id, vm->pc - 2, vm->sp);
    vm->natives[id](vm);
}

const OpHandler op_handlers[OP_COUNT] = {
    [HLT] = op_hlt,
    [PSH] = op_psh,
    [ADD] = op_add,
    [SUB] = op_sub,
    [MUL] = op_mul,
    [LOAD] = op_load,
    [STORE] = op_store,
    [JMP] = op_jmp,
    [JNZ] = op_jnz,
    [GT] = op_gt,
    [GTE] = op_gte,
    [LT] = op_lt,
    [LTE] = op_lte,
    [EQ] = op_eq,
    [NEQ] = op_neq,
    [CALL] = op_call,
    [RET] = op_ret,
    [WAIT] = op_wait,
    [INVOKE_STATIC] = op_invoke_static,
    [INVOKE_NATIVE] = op_invoke_native,
};
