#include <que/value.h>

#include <string.h>
#include <stdio.h>

#include "memory.h"
#include <que/table.h>
#include "value_internal.h"

Que_Object *allocate_obj(size_t size, Que_Type type) {
        Que_Object *obj = NULL;

        assert(size > sizeof(Que_Object));

        obj = ALLOCATE(obj, size);
        obj->next = NULL;
        obj->marked = QUE_FALSE;
        obj->type = type;

        return obj;
}

void free_obj(Que_Object *obj) {
        switch (obj->type) {
        case QUE_TYPE_STRING: {
                Que_StringObject *str = (Que_StringObject *)obj;

                str->str = FREE(str->str, str->length + 1);
                str = FREE(str, sizeof(Que_StringObject));
        } break;

        case QUE_TYPE_FUNCTION: {
                Que_FunctionObject *func = (Que_FunctionObject *)obj;

                free_obj((Que_Object *)func->name);
                chunk_free(&(func->code));
        } break;

        case QUE_TYPE_TABLE: {
                Que_TableObject *tab = (Que_TableObject *)obj;
                Que_DeleteTable(tab);
        } break;

        default: {} break;
        }
}

Que_StringObject *allocate_string(const char *str, size_t length) {
        Que_StringObject *obj = (Que_StringObject *)allocate_obj(
                sizeof(Que_StringObject), QUE_TYPE_STRING
        );

        obj->str = ALLOCATE(obj->str, length + 1);

        obj->length = length;
        memcpy(obj->str, str, length);
        obj->str[obj->length] = '\0';

        return obj;
}

Que_FunctionObject *allocate_function(int arity, const char *name, size_t namelength) {
        Que_FunctionObject *obj = (Que_FunctionObject *)allocate_obj(
                sizeof(Que_FunctionObject), QUE_TYPE_FUNCTION
        );

        obj->arity = arity;
        obj->name = allocate_string(name, namelength);
        chunk_init(&(obj->code));

        return obj;
}

void Que_ValueNil(Que_Value *val) {
        val->type = QUE_TYPE_NIL;
        val->value.o = NULL;
}

void Que_ValueChar(Que_Value *val, char c) {
        val->type = QUE_TYPE_CHAR;
        val->value.c = c;
}

void Que_ValueBool(Que_Value *val, int b) {
        val->type = QUE_TYPE_BOOL;
        val->value.b = b;
}

void Que_ValueInt(Que_Value *val, Que_Int i) {
        val->type = QUE_TYPE_INT;
        val->value.i = i;
}

void Que_ValueFloat(Que_Value *val, Que_Float f) {
        val->type = QUE_TYPE_FLOAT;
        val->value.f = f;
}

void Que_ValueString(Que_Value *val, const char *str, size_t len) {
        val->type = QUE_TYPE_STRING;
        val->value.o = (Que_Object *)allocate_string(str, len);
}

void Que_ValueTable(Que_Value *val, struct Que_TableObject *table) {
        val->type = QUE_TYPE_TABLE;
        val->value.o = (Que_Object *)table;
}

void Que_ValueFunction(Que_Value *val, Que_FunctionObject *func) {
        val->type = QUE_TYPE_FUNCTION;
        val->value.o = (Que_Object *)func;
}

void Que_ValueCFunction(Que_Value *val, Que_CFunction callback) {
        val->type = QUE_TYPE_CFUNCTION;
        val->value.o = (Que_Object *)callback;
}
