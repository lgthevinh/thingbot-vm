#include "opcode.h"
#include "vm.h"
#include <stdio.h>

void app_main(void) {
    const int prog[] = {
        PSH, 10,  // x = 10
        STORE, 0, // local[0] = x
        LOAD, 0,  // copy x from local[0] to stack
        PSH, 1000,
        GT,      // compare a (x) with b (1000)
        JNZ, 20, // if not zero, jump to halt
        LOAD, 0, // GT consume x, so get x again
        PSH, 5,
        MUL,      // x * 5
        STORE, 0, // pop the value of stack into local[0]
        JMP, 4,   // jump to prog[4]
        HLT,
    };

    vm_t vm;
    vm_init(&vm, prog);
    vm_run(&vm);
}
