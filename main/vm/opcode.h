#pragma once

// Stack-base vm opcode
typedef enum {
    HLT,
    PSH,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    LOAD,
    STORE,
    JMP,
    JNZ,
    GT,
    GTE,
    LT,
    LTE,
    EQ,
    NEQ,
    CALL,
    RET,
    WAIT,
    INVOKE_STATIC,
    INVOKE_NATIVE,

    // len on vm opcode
    OP_COUNT,
} OpCode;