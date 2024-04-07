#ifndef QUE_STATE_INTERNAL_H
#define QUE_STATE_INTERNAL_H

#include <que/state.h>
#include "memory.h"
#include "value_internal.h"

#include <stdio.h>

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

void print_stack(Que_State *state, const char *title);

void stack_push(Que_State *state, Que_Value *val);

Que_Value *stack_pop(Que_State *state);

void stack_set(Que_State *state, int offset, Que_Value *value);

Que_Value *stack_peek(Que_State *state, int offset);

#endif /* QUE_STATE_INTERNAL_H */
