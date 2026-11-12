#pragma once

#include <stdbool.h>

#define VM_LOCAL_SLOT 16 // local slots
#define VM_STACK_LEN 256 // operand stack slots
#define VM_CALL_DEPTH 8  // maximum nested calls

// Simple stack-based VM on ThingBot (ESP32C3)
typedef struct {
    const int *prog;               // program being run
    int ip;                        // index of the next slot to fetch
    int sp;                        // index of the operand stack top, -1 when empty
    int stack[VM_STACK_LEN];       // operand stack, grows upward

    int call_sp;                   // index of the call stack top, -1 when empty
    int call_stack[VM_CALL_DEPTH]; // return addresses

    int local[VM_LOCAL_SLOT];      // shared by the whole program, kept between runs
    bool running;                  // the run loop continues while true

    void *ctx;                     // board context, passed to vm_init, never read by the VM core
} vm_t;

void vm_init(vm_t *vm, void *ctx);
void vm_run(vm_t *vm, const int *prog);

static inline int vm_fetch(vm_t *vm) {
    return vm->prog[vm->ip++];
}

static inline void vm_push(vm_t *vm, int val) {
    vm->stack[++vm->sp] = val;
}

static inline int vm_pop(vm_t *vm) {
    return vm->stack[vm->sp--];
}

static inline void vm_callstack_push(vm_t *vm, int ip) {
    vm->call_stack[++vm->call_sp] = ip;
}

static inline int vm_callstack_pop(vm_t *vm) {
    return vm->call_stack[vm->call_sp--];
}
