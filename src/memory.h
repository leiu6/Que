#ifndef QUE_MEMORY_H
#define QUE_MEMORY_H

#include <que/common.h>

void *reallocate(void *buf, size_t old_size, size_t new_size);

#define ALLOCATE(buf, size) reallocate(buf, 0, size)
#define FREE(buf, size) reallocate(buf, size, 0)
#define ARRAY_GROW(array, old_size, new_size) reallocate(array, old_size, new_size)

#endif /* QUE_MEMORY_H */
