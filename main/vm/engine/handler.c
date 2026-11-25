#include "handler.h"
#include "esp_log.h"
#include <limits.h>
#include <stdbool.h>

static const char *TAG = "vm";

static bool check_global(engine_t *e, int idx) {
    if (idx >= 0 && idx < VM_GLOBAL_SLOT) {
        return true;
    }

    ESP_LOGE(TAG, "bad global idx %d, pc %d", idx, e->pc - 2);
    e->status = ENGINE_HALTED;
    return false;
}

static void op_hlt(engine_t *e, const int *prog) {
    e->status = ENGINE_HALTED;

    ESP_LOGI(TAG, "halt, pc %d", e->pc - 1);
}

static void op_psh(engine_t *e, const int *prog) {
    int val = engine_fetch(e, prog);
    engine_push(e, val);

    ESP_LOGD(TAG, "psh %d, pc %d, sp %d", val, e->pc - 2, e->sp);
}

static void op_add(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a + b);

    ESP_LOGD(TAG, "add %d + %d = %d, pc %d, sp %d", a, b, a + b, e->pc - 1, e->sp);
}

static void op_sub(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a - b);

    ESP_LOGD(TAG, "sub %d - %d = %d, pc %d, sp %d", a, b, a - b, e->pc - 1, e->sp);
}

static void op_mul(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a * b);

    ESP_LOGD(TAG, "mul %d * %d = %d, pc %d, sp %d", a, b, a * b, e->pc - 1, e->sp);
}

static bool check_divisor(engine_t *e, int b) {
    if (b != 0) {
        return true;
    }

    ESP_LOGE(TAG, "division by zero, pc %d", e->pc - 1);
    e->status = ENGINE_HALTED;
    return false;
}

static void op_div(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    if (!check_divisor(e, b)) {
        return;
    }

    // INT_MIN / -1 overflows, wrap it to INT_MIN
    int q = (a == INT_MIN && b == -1) ? INT_MIN : a / b;
    engine_push(e, q);

    ESP_LOGD(TAG, "div %d / %d = %d, pc %d, sp %d", a, b, q, e->pc - 1, e->sp);
}

static void op_mod(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    if (!check_divisor(e, b)) {
        return;
    }

    // INT_MIN % -1 overflows, the remainder of any division by -1 is 0
    int r = (b == -1) ? 0 : a % b;
    engine_push(e, r);

    ESP_LOGD(TAG, "mod %d %% %d = %d, pc %d, sp %d", a, b, r, e->pc - 1, e->sp);
}

static void op_gload(engine_t *e, const int *prog) {
    int idx = engine_fetch(e, prog);
    if (!check_global(e, idx)) {
        return;
    }
    engine_push(e, e->global[idx]);

    ESP_LOGD(TAG, "gload [%d] = %d, pc %d, sp %d", idx, e->global[idx], e->pc - 2, e->sp);
}

static void op_gstore(engine_t *e, const int *prog) {
    int idx = engine_fetch(e, prog);
    if (!check_global(e, idx)) {
        return;
    }
    e->global[idx] = engine_pop(e);

    ESP_LOGD(TAG, "gstore [%d] = %d, pc %d, sp %d", idx, e->global[idx], e->pc - 2, e->sp);
}

static void op_jmp(engine_t *e, const int *prog) {
    int pc = engine_fetch(e, prog);

    ESP_LOGD(TAG, "jmp %d -> %d, sp %d", e->pc - 2, pc, e->sp);
    e->pc = pc;
}

static void op_jnz(engine_t *e, const int *prog) {
    int target_pc = engine_fetch(e, prog);

    // pop the condition when read
    // not to hold the condition in the stack, avoid stack overflow in loops
    int cond = engine_pop(e);
    if (cond == 0) {
        ESP_LOGD(TAG, "jnz not taken, pc %d, sp %d", e->pc - 2, e->sp);
        return;
    }

    ESP_LOGD(TAG, "jnz %d -> %d, sp %d", e->pc - 2, target_pc, e->sp);
    e->pc = target_pc;
}

static void op_gt(engine_t *e, const int *prog) {
    // pop b and a from stack, then psh:
    // 1 if a is greater than b, else 0
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a > b);

    ESP_LOGD(TAG, "gt %d > %d = %d, pc %d, sp %d", a, b, a > b, e->pc - 1, e->sp);
}

static void op_gte(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a >= b);

    ESP_LOGD(TAG, "gte %d >= %d = %d, pc %d, sp %d", a, b, a >= b, e->pc - 1, e->sp);
}

static void op_lt(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a < b);

    ESP_LOGD(TAG, "lt %d < %d = %d, pc %d, sp %d", a, b, a < b, e->pc - 1, e->sp);
}

static void op_lte(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a <= b);

    ESP_LOGD(TAG, "lte %d <= %d = %d, pc %d, sp %d", a, b, a <= b, e->pc - 1, e->sp);
}

static void op_eq(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a == b);

    ESP_LOGD(TAG, "eq %d == %d = %d, pc %d, sp %d", a, b, a == b, e->pc - 1, e->sp);
}

static void op_neq(engine_t *e, const int *prog) {
    int b = engine_pop(e);
    int a = engine_pop(e);
    engine_push(e, a != b);

    ESP_LOGD(TAG, "neq %d != %d = %d, pc %d, sp %d", a, b, a != b, e->pc - 1, e->sp);
}

static void op_call(engine_t *e, const int *prog) {
    int target_pc = engine_fetch(e, prog);
    if (e->call_sp >= VM_CALL_DEPTH - 1) {
        ESP_LOGE(TAG, "call stack overflow, pc %d", e->pc - 2);
        e->status = ENGINE_HALTED;
        return;
    }

    // pc already points past the operand, which is the return address
    engine_callstack_push(e, e->pc);
    e->pc = target_pc;
}

static void op_ret(engine_t *e, const int *prog) {
    if (e->call_sp < 0) {
        ESP_LOGE(TAG, "ret with empty call stack, pc %d", e->pc - 1);
        e->status = ENGINE_HALTED;
        return;
    }

    e->pc = engine_callstack_pop(e);
}

static void op_wait(engine_t *e, const int *prog) {
    int ms = engine_pop(e);
    if (ms < 0) {
        ms = 0;
    }

    // hand the pause to the caller of vm_run, so the engine does not depend on FreeRTOS
    e->wait_ms = ms;
    e->status = ENGINE_WAITING;

    ESP_LOGD(TAG, "wait %d ms, pc %d, sp %d", ms, e->pc - 1, e->sp);
}

static void op_mload8(engine_t *e, const int *prog) {
    int addr = engine_pop(e);
    uint8_t *p = engine_mem(e, addr, 1);
    if (p == NULL) {
        return;
    }
    engine_push(e, p[0]);

    ESP_LOGD(TAG, "mload8 [%d] = %d, pc %d, sp %d", addr, p[0], e->pc - 1, e->sp);
}

static void op_mstore8(engine_t *e, const int *prog) {
    int byte = engine_pop(e);
    int addr = engine_pop(e);
    uint8_t *p = engine_mem(e, addr, 1);
    if (p == NULL) {
        return;
    }
    p[0] = (uint8_t)byte;

    ESP_LOGD(TAG, "mstore8 [%d] = %d, pc %d, sp %d", addr, p[0], e->pc - 1, e->sp);
}

static void op_mload32(engine_t *e, const int *prog) {
    int addr = engine_pop(e);
    uint8_t *p = engine_mem(e, addr, 4);
    if (p == NULL) {
        return;
    }

    // assemble little-endian byte by byte, since addresses need no alignment
    uint32_t w = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    engine_push(e, (int)w);

    ESP_LOGD(TAG, "mload32 [%d] = %d, pc %d, sp %d", addr, (int)w, e->pc - 1, e->sp);
}

static void op_mstore32(engine_t *e, const int *prog) {
    int word = engine_pop(e);
    int addr = engine_pop(e);
    uint8_t *p = engine_mem(e, addr, 4);
    if (p == NULL) {
        return;
    }

    uint32_t w = (uint32_t)word;
    p[0] = (uint8_t)w;
    p[1] = (uint8_t)(w >> 8);
    p[2] = (uint8_t)(w >> 16);
    p[3] = (uint8_t)(w >> 24);

    ESP_LOGD(TAG, "mstore32 [%d] = %d, pc %d, sp %d", addr, word, e->pc - 1, e->sp);
}

static void op_invoke_static(engine_t *e, const int *prog) {
    ESP_LOGE(TAG, "invoke static not implemented, pc %d", e->pc - 1);
    e->status = ENGINE_HALTED;
}

const OpHandler op_handlers[OP_COUNT] = {
    [HLT] = op_hlt,
    [PSH] = op_psh,
    [ADD] = op_add,
    [SUB] = op_sub,
    [MUL] = op_mul,
    [DIV] = op_div,
    [MOD] = op_mod,
    [GLOAD] = op_gload,
    [GSTORE] = op_gstore,
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
    [MLOAD8] = op_mload8,
    [MSTORE8] = op_mstore8,
    [MLOAD32] = op_mload32,
    [MSTORE32] = op_mstore32,
};
