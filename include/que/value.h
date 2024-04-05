#ifndef QUE_VALUE_H
#define QUE_VALUE_H

#include "common.h"

typedef enum {
	QUE_TYPE_NIL,
	QUE_TYPE_CHAR,
	QUE_TYPE_BOOL,
	QUE_TYPE_INT,
	QUE_TYPE_FLOAT,
	QUE_TYPE_STRING,
	QUE_TYPE_TABLE,
	QUE_TYPE_FUNCTION,
	QUE_TYPE_CFUNCTION
} Que_Type;

typedef struct Que_Object Que_Object;

typedef struct {
	Que_Type type;

	union{
		char c;
		Que_Byte b;
		Que_Int i;
		Que_Float f;
		Que_Object *o;
	} value;
} Que_Value;

struct Que_Object {
	Que_Type type;
	Que_Object *next;
	Que_Byte marked;
};

#define QUE_OBJECT_HEAD Que_Object ob_head


Que_Object *allocate_obj(size_t size, Que_Type type);

void free_obj(Que_Object *obj);

typedef struct {
	QUE_OBJECT_HEAD;

	size_t length;
	char *str;
} Que_StringObject;

Que_StringObject *allocate_string(const char *str, size_t length);

typedef struct Que_FunctionObject Que_FunctionObject;

Que_FunctionObject *allocate_function(Que_Value *identifier);

struct Que_State;
typedef int (*Que_CFunction) (struct Que_State *, int);

void Que_ValueCFunction(Que_Value *val, Que_CFunction callback);

struct Que_TableObject;

void Que_ValueNil(Que_Value *val);
void Que_ValueChar(Que_Value *val, char c);
void Que_ValueBool(Que_Value *val, int b);
void Que_ValueInt(Que_Value *val, Que_Int i);
void Que_ValueFloat(Que_Value *val, Que_Float f);
void Que_ValueString(Que_Value *val, const char *str, size_t len);
void Que_ValueTable(Que_Value *val, struct Que_TableObject *table);
void Que_ValueFunction(Que_Value *val, Que_FunctionObject *func);

#endif /* QUE_VALUE_H */
