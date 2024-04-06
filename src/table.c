#include <que/table.h>

#include "memory.h"

#include <stdio.h>
#include <string.h>

typedef unsigned long int Hash;

#define QUE_TABLE_ROWS (QUE_BYTE_MAX + 1)
#define QUE_TABLE_COLS (QUE_BYTE_MAX + 1)

typedef struct TableEntry {
        struct TableEntry *next;
        Hash key;
        Que_Value val;
} TableEntry;

struct Que_TableObject {
        QUE_OBJECT_HEAD;

        TableEntry *data[QUE_TABLE_ROWS][QUE_TABLE_COLS];
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
        Que_TableObject *table = (Que_TableObject *)allocate_obj(sizeof(Que_TableObject), QUE_TYPE_TABLE);
        size_t i;

        for (i = 0; i < QUE_TABLE_ROWS; i++) {
                size_t j;
                for (j = 0; j < QUE_TABLE_COLS; j++) {
                        table->data[i][j] = NULL;
                }
        }

        return table;
}

void Que_DeleteTable(Que_TableObject *table) {
        size_t i;
        for (i = 0; i < QUE_TABLE_ROWS; i++) {
                size_t j;
                for (j = 0; j < QUE_TABLE_COLS; j++) {
                        TableEntry *e = table->data[i][j];

                        for (;;) {
                                if (e) {
                                        TableEntry *del = e;
                                        e = e->next;
                                        del = FREE(del, sizeof(TableEntry));
                                } else {
                                        break;
                                }
                        }
                }
        }

        table = FREE(table, sizeof(Que_TableObject));
}

#define NTH_BYTE(num, n) \
        ((num >> (8 * n)) & 0xff)

static void insert_at_end(TableEntry *begin, TableEntry *insert) {
        TableEntry *e = begin;
        for (;;) {
                if (e->next == NULL) {
                        e->next = insert;
                        break;
                }
                e = e->next;
        }
}

static TableEntry *table_contains(Que_TableObject *table, Que_Value *key) {
	Hash hash = hash_value(key);
        TableEntry *found = NULL;
        Que_Byte row, col;

        row = NTH_BYTE(hash, 0);
        col = NTH_BYTE(hash, 1);

        found = table->data[row][col];

        if (found) {
                while (found != NULL) {
                        if (found->key == hash) {
                                return found;
                        } else {
                                found = found->next;
                        }
                }
                return NULL;
        } else {
                return NULL;
        }
}

void Que_TableInsert(Que_TableObject *table, Que_Value *key, Que_Value *value) {
        Que_Byte row, col;
        Hash hash = hash_value(key);
        TableEntry *tab_entry = NULL;
        TableEntry *found = NULL;

        tab_entry = ALLOCATE(tab_entry, sizeof(TableEntry));

        tab_entry->key = hash;
        tab_entry->next = NULL;
        tab_entry->val = *value;

        row = NTH_BYTE(hash, 0);
        col = NTH_BYTE(hash, 1);

        found = table_contains(table, key);

        if (found) {
                insert_at_end(found, tab_entry);
        } else {
                table->data[row][col] = tab_entry;
        }
}

void Que_TableQInsert(Que_TableObject *table, Que_Value *value, const char *key) {
        Que_Byte row, col;
        Hash hash = fnv_hash(key, strlen(key));
        TableEntry *tab_entry = NULL;
        TableEntry *found = NULL;

        tab_entry = ALLOCATE(tab_entry, sizeof(TableEntry));
        tab_entry->key = hash;
        tab_entry->next = NULL;
        tab_entry->val = *value;

        row = NTH_BYTE(hash, 0);
        col = NTH_BYTE(hash, 1);

        found = table->data[row][col];

        if (found) {
                insert_at_end(found, tab_entry);
        } else {
                table->data[row][col] = tab_entry;
        }
}

Que_Value *Que_TableGet(Que_TableObject *table, Que_Value *key) {
        TableEntry *entry = table_contains(table, key);
	return (entry) ? &(entry->val) : NULL;
}
