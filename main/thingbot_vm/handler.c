#include "handler.h"
#include "esp_log.h"

static const char *TAG = "vm";

static bool vm_check_local(VM *vm, int idx) {
    if (idx >= 0 && idx < VM_LOCALS) {
        return true;
    }

    ESP_LOGE(TAG, "bad local idx %d, ip %d", idx, vm->ip - 2);
    vm->running = false;
    return false;
}

static void op_hlt(VM *vm) {
    vm->running = false;

    ESP_LOGI(TAG, "halt, ip %d", vm->ip - 1);
}

static void op_psh(VM *vm) {
    int val = vm_fetch(vm);
    vm_push(vm, val);

    ESP_LOGD(TAG, "psh %d, ip %d, sp %d", val, vm->ip - 2, vm->sp);
}

static void op_add(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a + b);

    ESP_LOGD(TAG, "add %d + %d = %d, ip %d, sp %d", a, b, a + b, vm->ip - 1, vm->sp);
}

static void op_sub(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a - b);

    ESP_LOGD(TAG, "sub %d - %d = %d, ip %d, sp %d", a, b, a - b, vm->ip - 1, vm->sp);
}

static void op_mul(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a * b);

    ESP_LOGD(TAG, "mul %d * %d = %d, ip %d, sp %d", a, b, a * b, vm->ip - 1, vm->sp);
}

static void op_load(VM *vm) {
    int idx = vm_fetch(vm);
    if (!vm_check_local(vm, idx)) {
        return;
    }
    vm_push(vm, vm->local[idx]);

    ESP_LOGD(TAG, "load [%d] = %d, ip %d, sp %d", idx, vm->local[idx], vm->ip - 2, vm->sp);
}

static void op_store(VM *vm) {
    int idx = vm_fetch(vm);
    if (!vm_check_local(vm, idx)) {
        return;
    }
    vm->local[idx] = vm_pop(vm);

    ESP_LOGD(TAG, "store [%d] = %d, ip %d, sp %d", idx, vm->local[idx], vm->ip - 2, vm->sp);
}

static void op_jmp(VM *vm) {
    int ip = vm_fetch(vm);

    ESP_LOGD(TAG, "jmp %d -> %d, sp %d", vm->ip - 2, ip, vm->sp);
    vm->ip = ip;
}

static void op_jnz(VM *vm) {
    int target_ip = vm_fetch(vm);

    // pop the condition when read
    // not to hold the condition in the stack, avoid stack overflow in loops
    int cond = vm_pop(vm);
    if (cond == 0) {
        ESP_LOGD(TAG, "jnz not taken, ip %d, sp %d", vm->ip - 2, vm->sp);
        return;
    }

    ESP_LOGD(TAG, "jnz %d -> %d, sp %d", vm->ip - 2, target_ip, vm->sp);
    vm->ip = target_ip;
}

static void op_gt(VM *vm) {
    // pop b and a from stack, then psh:
    // 1 if a is greater than b, else 0
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a > b);

    ESP_LOGD(TAG, "gt %d > %d = %d, ip %d, sp %d", a, b, a > b, vm->ip - 1, vm->sp);
}

static void op_gte(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a >= b);

    ESP_LOGD(TAG, "gte %d >= %d = %d, ip %d, sp %d", a, b, a >= b, vm->ip - 1, vm->sp);
}

static void op_lt(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a < b);

    ESP_LOGD(TAG, "lt %d < %d = %d, ip %d, sp %d", a, b, a < b, vm->ip - 1, vm->sp);
}

static void op_lte(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a <= b);

    ESP_LOGD(TAG, "lte %d <= %d = %d, ip %d, sp %d", a, b, a <= b, vm->ip - 1, vm->sp);
}

static void op_eq(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a == b);

    ESP_LOGD(TAG, "eq %d == %d = %d, ip %d, sp %d", a, b, a == b, vm->ip - 1, vm->sp);
}

static void op_neq(VM *vm) {
    int b = vm_pop(vm);
    int a = vm_pop(vm);
    vm_push(vm, a != b);

    ESP_LOGD(TAG, "neq %d != %d = %d, ip %d, sp %d", a, b, a != b, vm->ip - 1, vm->sp);
}

static void op_call(VM *vm) {
    int target_ip = vm_fetch(vm);
    if (vm->call_sp >= VM_CALL_DEPTH - 1) {
        ESP_LOGE(TAG, "call stack overflow, ip %d", vm->ip - 2);
        vm->running = false;
        return;
    }

    // ip already points past the operand, which is the return address
    vm_callstack_push(vm, vm->ip);
    vm->ip = target_ip;
}

static void op_ret(VM *vm) {
    if (vm->call_sp < 0) {
        ESP_LOGE(TAG, "ret with empty call stack, ip %d", vm->ip - 1);
        vm->running = false;
        return;
    }

    vm->ip = vm_callstack_pop(vm);
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
};
