#include "state_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "opcodes.h"
#include "parser.h"
#include "lexer.h"
#include "stdlib/stdlibs.h"

#define DEFAULT_STACK_SIZE (256 * 256)
#define DEFAULT_MAX_RECURSION 256

Que_State *Que_NewStateEx(size_t stack_size, size_t max_recursion) {
        Que_State *state = NULL;
        Que_Value *stack = NULL;
        CallFrame *frames = NULL;

        state = malloc(sizeof(Que_State));
        if (!state) {
                goto cleanup;
        }

        stack = malloc(sizeof(Que_Value) * stack_size);
        if (!stack) {
                goto cleanup;
        }
        state->stack = stack;
        state->stack_top = state->stack;
        state->stack_size = stack_size;

        frames = malloc(sizeof(CallFrame) * max_recursion);
        if (!frames) {
                goto cleanup;
        }
        state->frames = frames;
        state->frame_current = state->frames;
        state->max_recursion = max_recursion;

        /* This function can never fail so no need to check */
        state->globals = Que_NewTable();

        return state;

cleanup:
        if (state) {
                free(state);
        }

        if (stack) {
                free(stack);
        }

        if (frames) {
                free(frames);
        }

        return NULL;
}

Que_State *Que_NewState(void) {
        return Que_NewStateEx(DEFAULT_STACK_SIZE, DEFAULT_MAX_RECURSION);
}

void Que_DeleteState(Que_State *state) {
        state->stack = FREE(state->stack, state->stack_size);
        state->frames = FREE(state->frames, state->max_recursion);
        state = FREE(state, sizeof(Que_State));
}

int Que_ExecuteString(Que_State *state, const char *str) {
        Que_FunctionObject *start = NULL;

        io_bootstrap(state);

#ifdef QUE_DEBUG_INSTRUCTIONS
        lexer_init(str);
        while (1) {
                Token tok;
                lexer_next(&tok);

                if (tok.type == TOK_INDENT) {
                        tok.start = "->";
                        tok.length = 2;
                }

                if (tok.type == TOK_DEDENT) {
                        tok.start = "<-";
                        tok.length = 2;
                }

                if (tok.type == TOK_EOL) {
                        tok.start = "\\n";
                        tok.length = 2;
                }

                printf("%zu, %zu [%s: %.*s]\n", 
                        lexer_line(),
                        lexer_col(),
                        TOKEN_NAMES[tok.type],
                        (int)tok.length, 
                        tok.start);

                if (tok.type == TOK_EOF) {
                        break;
                }
        }
        puts("");
#endif


        parser_init("<user>", str);
        start = parser_parse();

        /* Setup the state */
        state->frame_current->func = start;
        state->frame_current->ip = start->code.code;
        state->frame_current->slots = state->stack_top;

        vm_execute(state);

        free_obj((Que_Object *)start);

        return 0;
}

Que_Type Que_GetType(Que_State *state, int offset) {
        return (state->stack_top + offset)->type;
}

Que_Type Que_GetValue(Que_State *state, Que_Value *out_value, int offset) {
        *out_value = *(state->stack_top + offset);
        return Que_GetType(state, offset);
}

int Que_IsNil(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_NIL;
}

int Que_IsChar(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_CHAR;
}

int Que_IsBool(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_BOOL;
}

int Que_IsInt(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_INT;
}

int Que_IsFloat(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_FLOAT;
}

int Que_IsString(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_STRING;
}

int Que_IsTable(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_TABLE;
}

int Que_IsFunction(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_FUNCTION;
}

int Que_IsCFunction(Que_State *state, int offset) {
        return Que_GetType(state, offset) == QUE_TYPE_CFUNCTION;
}

static Que_Value *stacktop(Que_State *state, int offset) {
        return state->stack_top + offset;
}

int Que_AsChar(Que_State *state, int offset, char *out_char) {
        Que_Value *top = stacktop(state, offset);

        if (Que_IsChar(state, offset)) {
                *out_char = top->value.c;
                return QUE_TRUE;
        }

        return QUE_FALSE;
}

int Que_AsInt(Que_State *state, int offset, Que_Int *out_int) {
        Que_Value *top = stacktop(state, offset);

        if (Que_IsInt(state, offset)) {
                *out_int = top->value.i;
                return QUE_TRUE;
        }
        
        return QUE_FALSE;
}

int Que_AsFloat(Que_State *state, int offset, Que_Float *out_float) {
        Que_Value *top = stacktop(state, offset);

        if (Que_IsFloat(state, offset)) {
                *out_float = top->value.f;
                return QUE_TRUE;
        }
        
        return QUE_FALSE;
}

int Que_AsString(Que_State *state, int offset, char **out_str, size_t *out_length) {
        Que_Value *top = stacktop(state, offset);

        if (Que_IsString(state, offset)) {
                *out_str = ((Que_StringObject *)top->value.o)->str;
                *out_length = ((Que_StringObject *)top->value.o)->length;
                return QUE_TRUE;
        }
        
        return QUE_FALSE;
}

void Que_PushNil(Que_State *state) {
        Que_Value val;
        Que_ValueNil(&val);
        stack_push(state, &val);
}

void Que_PushChar(Que_State *state, char c) {
        Que_Value val;
        Que_ValueChar(&val, c);
        stack_push(state, &val);
}

void Que_PushBool(Que_State *state, int b) {
        Que_Value val;
        assert(b == QUE_TRUE || b == QUE_FALSE);
        Que_ValueBool(&val, b);
        stack_push(state, &val);
}

void Que_PushInt(Que_State *state, Que_Int i) {
        Que_Value val;
        Que_ValueInt(&val, i);
        stack_push(state, &val);
}

void Que_PushFloat(Que_State *state, Que_Float f) {
        Que_Value val;
        Que_ValueFloat(&val, f);
        stack_push(state, &val);
}

void Que_PushString(Que_State *state, const char *str) {
        Que_Value val;
        Que_ValueString(&val, str, strlen(str));
        stack_push(state, &val);
}

void Que_PushCFunction(Que_State *state, Que_CFunction func) {
        Que_Value val;
        Que_ValueCFunction(&val, func);
        stack_push(state, &val);
}

void Que_SetGlobal(Que_State *state, int offset, const char *name) {
        Que_Value key;
        Que_ValueString(&key, name, strlen(name));
        Que_TableInsert(state->globals, &key, state->stack_top + offset);
}

int Que_GetGlobal(Que_State *state, const char *name) {
        Que_Value key, *val;
        Que_ValueString(&key, name, strlen(name));

        val = Que_TableGet(state->globals, &key);

        if (val) {
                /* TODO: add limits check */
                stack_push(state, val);
                return QUE_TRUE;
        }

        return QUE_FALSE;
}

Que_Value *Que_PopValue(Que_State *state) {
        return stack_pop(state);
}

void Que_LoadTable(Que_State *state, Que_TableObject *table, const char *name) {
        Que_Value tabval;
        Que_ValueTable(&tabval, table);
        stack_push(state, &tabval);
        Que_SetGlobal(state, -1, name);
        /* Que_PopValue(state); */
        /* Que_PopValue(state); */
}

void print_stack(Que_State *state, const char *title) {
        Que_Value *cur;

        printf("Stack: %s\n", title);
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

void stack_push(Que_State *state, Que_Value *val) {
        /* TODO: check stack limits */
        *state->stack_top++ = *val;

#ifdef QUE_DEBUG_STACK
        print_stack(state, "push");
#endif

        assert(state->stack_top <= state->stack + state->stack_size);
}

Que_Value *stack_pop(Que_State *state) {
        Que_Value *result = --state->stack_top;
        assert(state->stack_top >= state->stack);

#ifdef QUE_DEBUG_STACK
        print_stack(state, "pop");
#endif

        return result;
}

void stack_set(Que_State *state, int offset, Que_Value *value) {
        state->stack_top[offset] = *value;

#ifdef QUE_DEBUG_STACK
        print_stack(state, "set");
#endif
}

Que_Value *stack_peek(Que_State *state, int offset) {
        return state->stack_top + offset;
}
