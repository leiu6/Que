#include "stdlibs.h"

#include <stdio.h>
#include <string.h>

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

        Que_PushNil(state);

        return 0;
}

int io_input(Que_State *state, int argc) {
        char line[2];

        if (argc != 0) {
                Que_PushString(state, "io.input accepts no arguments");
                Que_PushNil(state);
                return -1;
        }

        if (!fgets(line, sizeof(line), stdin)) {
                puts("");
                Que_PushString(state, "input string was too long");
                Que_PushNil(state);
                return -1;
        }
        line[strcspn(line, "\n")] = 0;

        Que_PushString(state, line);
        return 0;
}

Que_TableMethodDef methods[] = {
        { "print", io_print },
        { "input", io_input },
        { NULL, NULL } /* Sentinel */
};

void io_bootstrap(Que_State *state) {
        Que_LoadLibrary(state, methods, "io");
}
