#include "chunk.h"

#include "memory.h"
#include "opcodes.h"
#include <que/value.h>

#include <stdio.h>
#include <string.h>

#define CODE_INIT_SIZE 64
#define CONSTANTS_INIT_SIZE 16

void chunk_init(Chunk *chunk) {
        chunk->code = ALLOCATE(chunk->code, CODE_INIT_SIZE);
        chunk->code_allocated = CODE_INIT_SIZE;
        chunk->code_size = 0;

        chunk->constants = ALLOCATE(chunk->constants, sizeof(Que_Value) * CONSTANTS_INIT_SIZE);
        chunk->constants_allocated = CONSTANTS_INIT_SIZE;
        chunk->constants_size = 0;

        memset(chunk->constants, 0xAA, chunk->constants_allocated);
}

void chunk_free(Chunk *chunk) {
        FREE(chunk->code, chunk->code_allocated);
        FREE(chunk->constants, chunk->constants_allocated);
}

void chunk_write_byte(Chunk *chunk, Que_Byte b) {
        if (chunk->code_size + 1 > chunk->code_allocated) {
                chunk->code = ARRAY_GROW(
                        chunk->code, 
                        chunk->code_allocated, 
                        chunk->code_allocated * 2
                );
                chunk->code_allocated *= 2;
        }

        chunk->code[chunk->code_size++] = b;
}

void chunk_write_word(Chunk *chunk, Que_Word w) {
        chunk_write_byte(chunk, w >> 8);
        chunk_write_byte(chunk, w & 0x00ff);
}

Que_Word chunk_write_constant(Chunk *chunk, Que_Value *v) {
        Que_Value val = *v;
        if (chunk->constants_size + 1 > chunk->constants_allocated) {
                chunk->constants = ARRAY_GROW(
                        chunk->constants,
                        chunk->constants_allocated,
                        chunk->constants_allocated * 2
                );
                chunk->constants_allocated *= 2;
        }

        chunk->constants[chunk->constants_size++] = val;

        return (Que_Word)(chunk->constants_size - 1);
}

void chunk_disassemble(const Chunk *chunk) {
        size_t i;

        for (i = 0; i < chunk->code_size; i++) {
                Que_Byte op = chunk->code[i];

                if (OPCODE_HAS_ARG[op]) {
                        Que_Word word = (chunk->code[i + 1] << 8) + chunk->code[i + 2];
                        printf("%04zu: %s, %d\n", i, OPCODE_NAMES[op], word);
                        i += 2;
                } else {
                        printf("%04zu: %s\n", i, OPCODE_NAMES[op]);
                }
        }
}
