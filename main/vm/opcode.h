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

    // thingbot specific opcode
    SET_BUZ,
    SET_BUZ_I,

    SET_DC,
    SET_DC_I,

    SET_I_DC,
    SET_I_DC_I,

    SET_SERVO,
    SET_SERVO_I,

    SET_I_SERVO,
    SET_I_SERVO_I,

    GET_SW,

    // len on vm opcode
    OP_COUNT,
} OpCode;