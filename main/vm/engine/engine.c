#include "engine.h"
#include "esp_log.h"
#include "handler.h"
#include <string.h>

static const char *TAG = "vm";

void engine_reset(engine_t *e) {
    e->pc = 0;
    e->sp = -1;
    e->call_sp = -1;
    e->status = ENGINE_RUNNING;

    // a program never sees the previous program's buffers
    memset(e->mem, 0, sizeof(e->mem));
}

uint8_t *engine_mem(engine_t *e, int ptr, int len) {
    // compare against VM_MEM_LEN - len so ptr + len cannot overflow
    if (ptr < 0 || len < 0 || len > VM_MEM_LEN || ptr > VM_MEM_LEN - len) {
        ESP_LOGE(TAG, "bad memory access ptr %d len %d", ptr, len);
        e->status = ENGINE_HALTED;
        return NULL;
    }

    return &e->mem[ptr];
}

static NativeFn find_native(int id, const native_table_t *common, const native_table_t *specific) {
    if (id < 0 || id > NATIVE_ID_MAX) {
        return NULL;
    }

    const native_table_t *table = common;
    int idx = id;
    if (id >= NATIVE_SPECIFIC_BASE) {
        table = specific;
        idx = id - NATIVE_SPECIFIC_BASE;
    }

    return idx < table->count ? table->fns[idx] : NULL;
}

static void invoke_native(engine_t *e, const int *prog, const native_table_t *common, const native_table_t *specific, runtime_t *rt) {
    int id = engine_fetch(e, prog);
    NativeFn fn = find_native(id, common, specific);
    if (fn == NULL) {
        ESP_LOGE(TAG, "bad native id 0x%04x, pc %d", id, e->pc - 2);
        e->status = ENGINE_HALTED;
        return;
    }

    ESP_LOGD(TAG, "invoke native 0x%04x, pc %d, sp %d", id, e->pc - 2, e->sp);
    fn(e, rt);
}

void engine_step(engine_t *e, const int *prog, const native_table_t *common, const native_table_t *specific, runtime_t *rt) {
    int opcode = engine_fetch(e, prog);

    // the only opcode that reaches the runtime, so it is dispatched here instead of through op_handlers
    if (opcode == INVOKE_NATIVE) {
        invoke_native(e, prog, common, specific, rt);
        return;
    }

    if (opcode < 0 || opcode >= OP_COUNT || op_handlers[opcode] == NULL) {
        ESP_LOGE(TAG, "bad opcode %d, pc %d", opcode, e->pc - 1);
        e->status = ENGINE_HALTED;
        return;
    }

    op_handlers[opcode](e, prog);
}
