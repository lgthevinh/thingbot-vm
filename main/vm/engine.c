#include "engine.h"
#include "esp_log.h"
#include "handler.h"

static const char *TAG = "vm";

void engine_reset(engine_t *e) {
    e->pc = 0;
    e->sp = -1;
    e->call_sp = -1;
    e->status = ENGINE_RUNNING;
}

static void invoke_native(engine_t *e, const int *prog, const NativeFn *natives, int native_count, runtime_t *rt) {
    int id = engine_fetch(e, prog);
    if (id < 0 || id >= native_count || natives[id] == NULL) {
        ESP_LOGE(TAG, "bad native id %d, pc %d", id, e->pc - 2);
        e->status = ENGINE_HALTED;
        return;
    }

    ESP_LOGD(TAG, "invoke native %d, pc %d, sp %d", id, e->pc - 2, e->sp);
    natives[id](e, rt);
}

void engine_step(engine_t *e, const int *prog, const NativeFn *natives, int native_count, runtime_t *rt) {
    int opcode = engine_fetch(e, prog);

    // the only opcode that reaches the runtime, so it is dispatched here instead of through op_handlers
    if (opcode == INVOKE_NATIVE) {
        invoke_native(e, prog, natives, native_count, rt);
        return;
    }

    if (opcode < 0 || opcode >= OP_COUNT || op_handlers[opcode] == NULL) {
        ESP_LOGE(TAG, "bad opcode %d, pc %d", opcode, e->pc - 1);
        e->status = ENGINE_HALTED;
        return;
    }

    op_handlers[opcode](e, prog);
}
