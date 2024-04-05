#ifndef QUE_TABLE_H
#define QUE_TABLE_H

#include "value.h"

typedef struct Que_TableObject Que_TableObject;

Que_TableObject *Que_NewTable(void);

void Que_DeleteTable(Que_TableObject *table);

void Que_TableInsert(Que_TableObject *table, Que_Value *key, Que_Value *value);

void Que_TableQInsert(Que_TableObject *table, Que_Value *value, const char *key);

Que_Value *Que_TableGet(Que_TableObject *table, Que_Value *key);

#endif /* QUE_TABLE_H */
