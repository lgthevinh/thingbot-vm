#pragma once

#include <stdbool.h>

#define VM_LOCALS 16
#define VM_STACK_LEN 256
#define VM_CALL_DEPTH 256

// Simple stack-based VM on ThingBot (ESP32C3)
typedef struct
{
    const int *prog;
    int ip;
    int sp;
    int stack[VM_STACK_LEN];

    int call_sp;
    int call_stack[VM_CALL_DEPTH];

    int local[VM_LOCALS];
    bool running;
} vm_t;

void vm_init(vm_t *vm, const int *prog);
void vm_run(vm_t *vm);

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