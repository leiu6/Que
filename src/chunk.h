#ifndef QUE_CHUNK_H
#define QUE_CHUNK_H

#include <que/common.h>
#include <que/value.h>

typedef struct {
        size_t code_allocated;
        size_t code_size;
        Que_Byte *code;

        size_t constants_allocated;
        size_t constants_size;
        Que_Value *constants;
} Chunk;

void chunk_init(Chunk *chunk);

void chunk_free(Chunk *chunk);

void chunk_write_byte(Chunk *chunk, Que_Byte b);

void chunk_write_word(Chunk *chunk, Que_Word w);

Que_Word chunk_write_constant(Chunk *chunk, Que_Value *v);

void chunk_disassemble(const Chunk *chunk);

#endif /* QUE_CHUNK_H */
