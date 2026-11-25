#pragma once

#include "runtime.h"

#define VM_GLOBAL_SLOT 16 // global slots
#define VM_STACK_LEN 256  // operand stack slots
#define VM_CALL_DEPTH 8   // maximum nested calls

typedef enum {
    ENGINE_HALTED,  // stopped by HLT or an error
    ENGINE_RUNNING, // executing instructions
    ENGINE_WAITING, // paused by WAIT for wait_ms, resumed by the next vm_run
} EngineStatus;

// Interpreter state, knows nothing about the board
struct engine_t {
    int pc;                        // index of the next slot to fetch
    int sp;                        // index of the operand stack top, -1 when empty
    int stack[VM_STACK_LEN];       // operand stack, grows upward

    int call_sp;                   // index of the call stack top, -1 when empty
    int call_stack[VM_CALL_DEPTH]; // return addresses

    int global[VM_GLOBAL_SLOT];    // shared by the whole program, kept between programs
    EngineStatus status;
    int wait_ms;                   // pause requested by the last WAIT
};

void engine_reset(engine_t *e);
void engine_step(engine_t *e, const int *prog, const native_table_t *common, const native_table_t *specific, runtime_t *rt);

static inline int engine_fetch(engine_t *e, const int *prog) {
    return prog[e->pc++];
}

static inline void engine_push(engine_t *e, int val) {
    e->stack[++e->sp] = val;
}

static inline int engine_pop(engine_t *e) {
    return e->stack[e->sp--];
}

static inline void engine_callstack_push(engine_t *e, int pc) {
    e->call_stack[++e->call_sp] = pc;
}

static inline int engine_callstack_pop(engine_t *e) {
    return e->call_stack[e->call_sp--];
}
