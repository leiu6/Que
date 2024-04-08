#include <que/table.h>

#include "memory.h"

#include <stdio.h>
#include <string.h>

typedef unsigned long int Hash;

#define NUM_BUCKETS (QUE_BYTE_MAX + 1)

typedef struct TableEntry {
        struct TableEntry *next;
        Hash key;
        Que_Value val;
} TableEntry;

struct Que_TableObject {
        QUE_OBJECT_HEAD;

        TableEntry *data[NUM_BUCKETS];
};

/* Hash function code from:
 * https://benhoyt.com/writings/hash-table-in-c/
 */

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static Hash fnv_hash(const void *memory, size_t size) {
        Hash hash = FNV_OFFSET;
        const char *byte;
        const char *buf = memory;

        for (byte = buf; byte <= buf + size; byte++) {
                hash ^= (Hash)(unsigned char)(*byte);
                hash *= FNV_PRIME;
        }
        return hash;
}

static Hash hash_value(Que_Value *value) {
        switch (value->type) {
        /* String gets a bit of a specialization here */
        case QUE_TYPE_STRING:
                return fnv_hash(((Que_StringObject *)value->value.o)->str, ((Que_StringObject *)value->value.o)->length);

        default:
                return fnv_hash(value, sizeof(Que_Value));
        }
}

Que_TableObject *Que_NewTable(void) {
        Que_TableObject *table = NULL;

        table = ALLOCATE(table, sizeof(Que_TableObject));
        memset(table->data, 0x00, sizeof(TableEntry *) * NUM_BUCKETS);

        return table;
}

static void bucket_delete(TableEntry *bucket) {
        TableEntry *cur = bucket;

        for (;;) {
                TableEntry *next;

                if (cur == NULL) {
                        /* Reached end */
                        return;
                }

                next = cur->next;
                cur = FREE(cur, sizeof(TableEntry));
                cur = next;
        }
}

void Que_DeleteTable(Que_TableObject *table) {
        size_t i;
        for (i = 0; i < NUM_BUCKETS; i++) {
                bucket_delete(table->data[i]);
        }

        table = FREE(table, sizeof(Que_TableObject));
}

#define getbucket(table, hash) (table->data[hash % NUM_BUCKETS])

static void insert_at_end(TableEntry *bucket, Hash hash, Que_Value *value) {
        TableEntry *cur = bucket;
        TableEntry *prev = NULL;

        for (;;) {
                if (!cur) {
                        TableEntry *insert = NULL;

                        insert = ALLOCATE(insert, sizeof(TableEntry));
                        insert->key = hash;
                        insert->val = *value;
                        insert->next = NULL;
                        assert(prev);
                        prev->next = insert;
                        return;
                }

                prev = cur;
                cur = cur->next;
        }
}

void Que_TableInsert(Que_TableObject *table, Que_Value *key, Que_Value *value) {
        Hash hash = hash_value(key);

        if (!getbucket(table, hash)) {
                getbucket(table, hash) = ALLOCATE(NULL, sizeof(TableEntry));
                getbucket(table, hash)->key = hash;
                getbucket(table, hash)->val = *value;
                getbucket(table, hash)->next = NULL;
                return;
        } else {
                insert_at_end(getbucket(table, hash), hash, value);
        }
}

void Que_TableQInsert(Que_TableObject *table, Que_Value *value, const char *key) {
        Que_Value str;
        Que_ValueString(&str, key, strlen(key));
        Que_TableInsert(table, &str, value);
}

TableEntry *find_in_bucket(TableEntry *bucket, Hash hash) {
        TableEntry *cur = bucket;

        for (;;) {
                if (!cur) {
                        return NULL;
                }

                if (cur->key == hash) {
                        return cur;
                }

                cur = cur->next;
        }
}

Que_Value *Que_TableGet(Que_TableObject *table, Que_Value *key) {
        Hash hash = hash_value(key);
        TableEntry *bucket = getbucket(table, hash);
        TableEntry *found = find_in_bucket(bucket, hash);

        return (found) ? &found->val : NULL;
}
