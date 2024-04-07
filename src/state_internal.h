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

static void print_stack(Que_State *state, const char *operation) {
        Que_Value *cur;

        printf("Stack: %s\n", operation);
        for (cur = state->stack; cur < state->stack_top; cur++) {
                switch (cur->type) {
                case QUE_TYPE_NIL:
                        puts("[nil: nil]");
                        break;

                case QUE_TYPE_CHAR:
                        printf("[char: %c]\n", cur->value.c);
                        break;

                case QUE_TYPE_BOOL: {
                        if (cur->value.b) {
                                puts("[bool: true]");
                        } else {
                        puts("[bool: false]");
                        }
                } break;

                case QUE_TYPE_INT: {
                        printf("[int: %lu]\n", cur->value.i);
                } break;

                case QUE_TYPE_FLOAT: {
                        printf("[float: %f]\n", cur->value.f);
                } break;

                case QUE_TYPE_STRING: {
                        printf("[string: %s]\n", ((Que_StringObject *)cur->value.o)->str);
                } break;

                case QUE_TYPE_TABLE: {
                        printf("[table: %p]\n", (void*)cur->value.o);
                } break;

                case QUE_TYPE_FUNCTION: {
                        printf("[function: %p]\n", (void*)cur->value.o);
                } break;

                case QUE_TYPE_CFUNCTION: {
                        printf("[cfunction: %p]\n", (void*)cur->value.o);
                } break;
                }
        }

        puts("");
}

static void stack_push(Que_State *state, Que_Value *val) {
        /* TODO: check stack limits */
        *state->stack_top++ = *val;

        print_stack(state, "push");

        assert(state->stack_top <= state->stack + state->stack_size);
}

static Que_Value *stack_pop(Que_State *state) {
        Que_Value *result = --state->stack_top;
        assert(state->stack_top >= state->stack);

        print_stack(state, "pop");

        return result;
}

#endif /* QUE_STATE_INTERNAL_H */
