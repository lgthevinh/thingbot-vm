#pragma once

// Stack-base vm opcode
typedef enum {
    HLT,
    PSH,
    ADD,
    SUB,
    MUL,
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

    // thingbot specific opcode
    MOTOR,
    SERVO,
    SPIN,
    BUZZ,
    LED,

    // len on vm opcode
    OP_COUNT,
} OpCode;