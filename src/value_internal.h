#ifndef QUE_VALUE_INTERNAL_H
#define QUE_VALUE_INTERNAL_H

#include "chunk.h"

struct Que_FunctionObject {
        QUE_OBJECT_HEAD;

        int arity;
        Que_StringObject *name;
        Chunk code;
};

#endif /* QUE_VALUE_INTERNAL_H */
