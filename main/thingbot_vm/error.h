#pragma once
#ifndef ERROR_H
#define ERROR_H

typedef enum
{
    SUCCESS,        // 0 as SUCCESS, c return 0 as success, this is reserve enum
    INTERNAL,       // internal implementation error of the VM
    STACK_OVERFLOW, // stack overflow error
    OUT_OF_MEM,     // out of memory error
    UNKNOWN         // unknown error
} Error;

#endif // ERROR_H
