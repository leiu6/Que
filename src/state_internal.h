#ifndef QUE_STATE_INTERNAL_H
#define QUE_STATE_INTERNAL_H

#include <que/state.h>
#include "memory.h"
#include "value_internal.h"

typedef struct {
        Que_FunctionObject *func;
        Que_Byte *ip;
        Que_Value *slots;
} CallFrame;

struct Que_State {
        Que_Value *stack;
        Que_Value *stack_top;
        size_t stack_size;

        CallFrame *frames;
        CallFrame *frame_current;
        size_t max_recursion;

        Que_TableObject *globals;
};

#endif /* QUE_STATE_INTERNAL_H */
