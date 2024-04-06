#include "vm.h"

#include "opcodes.h"
#include "state_internal.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

static const char *QUE_TYPE_NAMES[] = {
        "nil",
        "char",
        "bool",
        "int",
        "float",
        "string",
        "table",
        "function",
        "cfunction"
};

#define GET_BYTE() (*(state->frame_current->ip++))

#define GET_CONSTANT(i) (state->frame_current->func->code.constants[i])

#define IS_ARITHMETIC(val) (val.type == QUE_TYPE_INT || val.type == QUE_TYPE_FLOAT)

#define AS_ARITHMETIC(val) ((val.type == QUE_TYPE_INT) ? (val.value.i) : (val.value.f))

static int value_is_truthy(Que_Value *v) {
        switch (v->type) {
        case QUE_TYPE_NIL: return QUE_FALSE;
        case QUE_TYPE_CHAR: return QUE_TRUE;
        case QUE_TYPE_BOOL: return v->value.b;
        case QUE_TYPE_INT: return (v->value.i) ? QUE_TRUE : QUE_FALSE;
        case QUE_TYPE_FLOAT: return (v->value.f) ? QUE_TRUE : QUE_FALSE;
        case QUE_TYPE_STRING: return QUE_TRUE;
        case QUE_TYPE_TABLE: return QUE_TRUE;
        case QUE_TYPE_FUNCTION: return QUE_TRUE;
        case QUE_TYPE_CFUNCTION: return QUE_TRUE;
        }
}

static Que_Word get_word(Que_State *state) {
        Que_Byte upper, lower;

        upper = GET_BYTE();
        lower = GET_BYTE();

        return (upper << 8) + lower;
}

static void stack_push(Que_State *state, Que_Value *val) {
        /* TODO: check stack limits */
        *state->stack_top++ = *val;

        assert(state->stack_top <= state->stack + state->stack_size);
}

static Que_Value *stack_pop(Que_State *state) {
        return (--state->stack_top);

        assert(state->stack_top >= state->stack);
}

static void error(const char *format, ...) {
        va_list args;

        va_start(args, format);

        fprintf(stderr, "[!] ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");

        va_end(args);
}

int vm_execute(Que_State *state) {
        for (;;) {
                Que_Byte ins = GET_BYTE();

                switch (ins) {
                case OP_PUSH: {
                        Que_Word addr = get_word(state);
                        Que_Value value = GET_CONSTANT(addr);
                        stack_push(state, &value);
                } break;

                case OP_PUSH_TRUE: {
                        Que_PushBool(state, QUE_TRUE);
                } break;

                case OP_PUSH_FALSE: {
                        Que_PushBool(state, QUE_FALSE);
                } break;

                case OP_PUSH_NIL: {
                        Que_PushNil(state);
                } break;

                case OP_POP: {
                        stack_pop(state);
                } break;

                case OP_ADD: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l + r);

                        } else if (IS_ARITHMETIC(lhs) && IS_ARITHMETIC(rhs)) {
                                Que_Float l, r;

                                l = AS_ARITHMETIC(lhs);
                                r = AS_ARITHMETIC(rhs);

                                Que_PushFloat(state, l + r);
                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '+'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_SUBTRACT: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l - r);

                        } else if (IS_ARITHMETIC(lhs) && IS_ARITHMETIC(rhs)) {
                                Que_Float l, r;

                                l = AS_ARITHMETIC(lhs);
                                r = AS_ARITHMETIC(rhs);

                                Que_PushFloat(state, l - r);
                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '-'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_MULTIPLY: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l * r);

                        } else if (IS_ARITHMETIC(lhs) && IS_ARITHMETIC(rhs)) {
                                Que_Float l, r;

                                l = AS_ARITHMETIC(lhs);
                                r = AS_ARITHMETIC(rhs);

                                Que_PushFloat(state, l * r);
                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '*'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_DIVIDE: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l / r);

                        } else if (IS_ARITHMETIC(lhs) && IS_ARITHMETIC(rhs)) {
                                Que_Float l, r;

                                l = AS_ARITHMETIC(lhs);
                                r = AS_ARITHMETIC(rhs);

                                Que_PushFloat(state, l / r);
                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '/'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_POW: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, powl(l, r));

                        } else if (IS_ARITHMETIC(lhs) && IS_ARITHMETIC(rhs)) {
                                Que_Float l, r;

                                l = AS_ARITHMETIC(lhs);
                                r = AS_ARITHMETIC(rhs);

                                Que_PushFloat(state, pow(l, r));
                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '**'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_NEGATE: {
                        Que_Value v;

                        v = *stack_pop(state);

                        if (v.type == QUE_TYPE_INT) {
                                Que_PushInt(state, -v.value.i);
                        } else if (IS_ARITHMETIC(v)) {
                                Que_PushFloat(state, -AS_ARITHMETIC(v));
                        }
                } break;

                case OP_BAND: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l & r);

                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '&'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_BOR: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l | r);

                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '|'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_BXOR: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l ^ r);

                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '^'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_LSHIFT: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l << r);

                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '<<'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_RSHIFT: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        if (lhs.type == QUE_TYPE_INT && rhs.type == QUE_TYPE_INT) {
                                Que_Int l, r;

                                l = lhs.value.i;
                                r = lhs.value.i;

                                Que_PushInt(state, l & r);

                        } else {
                                error(
                                        "Invalid operands '%s' and '%s' for operator '>>'", 
                                        QUE_TYPE_NAMES[lhs.type], 
                                        QUE_TYPE_NAMES[rhs.type]
                                );
                                return -1;
                        }
                } break;

                case OP_BNOT: {
                        Que_Value val;
                        Que_Int i;

                        val = *stack_pop(state);

                        if (Que_AsInt(state, 0, &i)) {
                                Que_PushInt(state, ~i);
                        } else {
                                error("Invalid operands '%s' for operator '~'", QUE_TYPE_NAMES[val.type]);
                        }
                } break;

                case OP_AND: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        Que_PushBool(state, 
                                value_is_truthy(&lhs) && value_is_truthy(&rhs)
                        );
                } break;

                case OP_OR: {
                        Que_Value lhs, rhs;

                        rhs = *stack_pop(state);
                        lhs = *stack_pop(state);

                        Que_PushBool(state, 
                                value_is_truthy(&lhs) || value_is_truthy(&rhs)
                        );
                } break;

                case OP_NOT: {
                        Que_Value val;

                        val = *stack_pop(state);

                        Que_PushBool(state, 
                                value_is_truthy(&val)
                        );
                } break;

                case OP_DEFINE_GLOBAL: {
                        Que_Word addr = get_word(state);
                        Que_Value key = GET_CONSTANT(addr);
                        Que_Value *value = stack_pop(state);
			
                        Que_TableInsert(state->globals, &key, value);
                } break;

                case OP_GET_GLOBAL: {
                        Que_Word addr = get_word(state);
                        Que_Value key = GET_CONSTANT(addr);
                        Que_Value *value = Que_TableGet(state->globals, &key);
                        if (value) {
                                stack_push(state, value);
                        } else {
                                error("Global variable '%s' does not exist", ((Que_StringObject *)key.value.o)->str);
                                return -1;
                        }
                } break;

                case OP_SET_GLOBAL: {
                        Que_Word addr = get_word(state);
                        Que_Value key = GET_CONSTANT(addr);
                        Que_Value value = *stack_pop(state);
                        Que_Value *existing;

                        /* See if it is in the table or not */
                        existing = Que_TableGet(state->globals, &key);
                        if (existing) {
                                /* We already have it */
                                *existing = value;
                        } else {
			        assert(0 && "Attempt to set nonexistent global");
                                Que_TableInsert(state->globals, &key, &value);
                        }
                } break;

                case OP_SET_LOCAL: {
                        Que_Word slot = get_word(state);
                        Que_Value *local = &(state->frame_current->slots[slot]);
                        Que_Value set_to = *stack_pop(state);
                        *local = set_to;
                } break;

                case OP_GET_LOCAL: {
                        Que_Word slot = get_word(state);
                        Que_Value *local = &(state->frame_current->slots[slot]);
                        stack_push(state, local);
                } break;


                case OP_CALL: {
                        Que_Word args = get_word(state);
                        Que_Value *value;

                        /* Might need -1 */
                        value = (state->stack_top - args - 1);

                        if (value->type == QUE_TYPE_CFUNCTION) {
                                int ret;
                                Que_CFunction cfunc = (Que_CFunction)value->value.o;

                                ret = cfunc(state, args);

                                if (ret != 0) {
                                        Que_Value errorstr = *(state->stack_top - 2);
                                        error("%s", ((Que_StringObject *)errorstr.value.o)->str);

                                        return ret;
                                }
                        } else if (value->type == QUE_TYPE_FUNCTION) {
                                Que_FunctionObject *func = ((Que_FunctionObject *)(state->stack_top - args - 1)->value.o);
                                assert(func->ob_head.type == QUE_TYPE_FUNCTION);

                                state->frame_current++;
                                assert(state->frame_current <= state->frames + state->max_recursion);
                                state->frame_current->func = func;
                                state->frame_current->ip = func->code.code;
                                state->frame_current->slots = state->stack_top - args - 1;
                        } else {
                                error("Object type '%s' is not a function", QUE_TYPE_NAMES[value->type]);
                                return -1;
                        }
                } break;

                case OP_TABLE_GET: {
                        Que_Value *key = stack_pop(state);
                        Que_Value *table = stack_pop(state);
                        Que_Object *keyobj;
                        Que_TableObject *tableobj;
                        Que_Value *result;

                        if (key->type != QUE_TYPE_STRING) {
                                error("Table must be indexed with identifier, not '%s'", QUE_TYPE_NAMES[key->type]);
                                return -1;
                        } else if (table->type != QUE_TYPE_TABLE) {
                                error("Cannot index non table objecst such as '%s'", QUE_TYPE_NAMES[table->type]);
                                return -1;
                        }

                        keyobj = (Que_Object *)key->value.o;
                        tableobj = (Que_TableObject *)table->value.o;

                        result = Que_TableGet(tableobj, key);
                        if (!result) {
                                Que_PushNil(state);
                        } else {
                                stack_push(state, result);
                        }
                } break;

                case OP_RETURN: {
                        if (state->frame_current == state->frames) {
                                /* Halt execution */
                                return 0;
                        }

                        *state->frame_current->slots = *(state->stack_top);

                        state->frame_current--;
                } break;

                case OP_HALT: {
                        return 0;
                } break;

                default: {
                        error("Unknown opcode %d", ins);
                        return -1;
                } break;
                }
        }
}
