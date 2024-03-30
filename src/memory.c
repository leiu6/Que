#include "memory.h"

#include <stdlib.h>
#include <stdio.h>

#include <que/common.h>

QUE_NORETURN static void enomem(void) {
        fprintf(stderr, "[!] Fatal: QueVM is out of memory. Now exiting.\n");
        exit(25);
}

void *reallocate(void *buf, size_t old_size, size_t new_size) {
        if ((old_size == 0) && (new_size > 0)) {
                buf = malloc(new_size);
                if (!buf) {
                        enomem();
                }
                return buf;
        } else if ((old_size > 0) && (new_size > 0)) {
                void *temp = realloc(buf, new_size);
                if (!temp) {
                        enomem();
                }
                return temp;
        } else if ((old_size > 0) && (new_size == 0)) {
                free(buf);
                return NULL;
        }

        assert(0 && "unreachable");
        return NULL;
}
