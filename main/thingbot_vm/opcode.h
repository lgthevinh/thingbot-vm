#ifndef OPCODE_H
#define OPCODE_H

// Stack-base vm opcode
typedef enum
{
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
    
    INVOKE,
    IRETURN,
} OpCode;

#endif // OPCODE_H