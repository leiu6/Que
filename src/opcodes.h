#ifndef QUE_OPCODES_H
#define QUE_OPCODES_H

#include <que/common.h>

/**
 * There are two types of opcodes in Que. There are 1 byte opcodes which do not
 * have an argument, and there are 3 byte opcodes which have 1 byte for the
 * opcode, and 2 bytes for the argument.
 * 
 * No-arg opcodes can be denoted by the OP(name) macro in opcodes.txt, while ones
 * with an arg can be denoted with OP_ARG(name).
*/

#define OP(name) name
#define OP_ARG(name) name
typedef enum {
#include "opcodes.txt"
} Op;
#undef OP
#undef OP_ARG

static const char *OPCODE_NAMES[] = {
#define OP(name) #name
#define OP_ARG(name) #name
#include "opcodes.txt"
#undef OP
#undef OP_ARG
};

static int OPCODE_HAS_ARG[] = {
#define OP(name) 0
#define OP_ARG(name) 1
#include "opcodes.txt"
#undef OP
#undef OP_ARG
};

#endif /* QUE_OPCODES_H */
