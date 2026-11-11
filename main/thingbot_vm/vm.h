#pragma once

#include <stdbool.h>

#define VM_LOCALS 16
#define VM_CALL_DEPTH 256

// Simple stack-based VM on ThingBot (ESP32C3)
typedef struct
{
    const int *prog;
    int ip;
    int sp;
    int stack[256];

    int call_sp;
    int call_stack[VM_CALL_DEPTH];

    int local[VM_LOCALS];
    bool running;
} VM;

void vm_init(VM *vm, const int *prog);
void vm_run(VM *vm);

static inline int vm_fetch(VM *vm) {
    return vm->prog[vm->ip++];
}

static inline void vm_push(VM *vm, int val) {
    vm->stack[++vm->sp] = val;
}

static inline int vm_pop(VM *vm) {
    return vm->stack[vm->sp--];
}

static inline void vm_callstack_push(VM *vm, int ip) {
    vm->call_stack[++vm->call_sp] = ip;
}

static inline int vm_callstack_pop(VM *vm) {
    return vm->call_stack[vm->call_sp--];
}