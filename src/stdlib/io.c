#include "stdlibs.h"

#include <stdio.h>

int io_print(Que_State *state, int argc) {
        Que_Value val;

        if (argc != 1) {
                Que_PushString(state, "print only accepts 1 argument");
                Que_PushNil(state);
                return -1;
        }

        switch (Que_GetValue(state, &val, -1)) {
        case QUE_TYPE_NIL:
                puts("nil");
                break;

        case QUE_TYPE_CHAR:
                printf("%c\n", val.value.c);
                break;

        case QUE_TYPE_BOOL: {
                if (val.value.b) {
                        puts("true");
                } else {
                        puts("false");
                }
        } break;

        case QUE_TYPE_INT: {
                printf("%lu\n", val.value.i);
        } break;

        case QUE_TYPE_FLOAT: {
                printf("%f\n", val.value.f);
        } break;

        case QUE_TYPE_STRING: {
                puts(((Que_StringObject *)(val.value.o))->str);
        } break;

        case QUE_TYPE_TABLE: {
                puts("<table>");
        } break;

        case QUE_TYPE_FUNCTION: {
                printf("<function>\n");
        } break;

        case QUE_TYPE_CFUNCTION: {
                printf("<cfunction: %p>\n", (void*)val.value.o);
        } break;

        }

        return 0;
}

void io_bootstrap(Que_State *state) {
        Que_TableObject *methods;
        Que_Value method_print;

        methods = Que_NewTable();

        Que_ValueCFunction(&method_print, io_print);
        Que_TableQInsert(methods, &method_print, "print");

        Que_LoadTable(state, methods, "io");
}
