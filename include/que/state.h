#ifndef QUE_STATE_H
#define QUE_STATE_H

#include "common.h"
#include "table.h"
#include "value.h"

/**
 * Defines the state of an instance of the Que interpreter. Every Que_State has
 * it's own call stack, value stack, parser, etc.
 */
typedef struct Que_State Que_State;

/**
 * Allocates a new Que_State. Returns NULL if the state could not be allocated.
 * When the user is done with the state, it must be freed by calling
 * Que_DeleteState(state).
 */
Que_State *Que_NewState(void);

/**
 * Works in the same way as Que_NewState, but allows the stack size and the
 * maximum recursion depth to be specified for the interpreter.
 */
Que_State *Que_NewStateEx(size_t stack_size, size_t max_recursion);

/**
 * Must be called when the user is done using the state. Frees any dynamic memory
 * or handles that are associated with it.
 */
void Que_DeleteState(Que_State *state);

/**
 * Executes the supplied string as Que code. Returns 0 if execution is successful
 * or a non-zero error code otherwise.
 */
int Que_ExecuteString(Que_State *state, const char *str);

Que_Type Que_GetType(Que_State *state, int offset);

Que_Type Que_GetValue(Que_State *state, Que_Value *out_value, int offset);

int Que_IsNil(Que_State *state, int offset);
int Que_IsChar(Que_State *state, int offset);
int Que_IsBool(Que_State *state, int offset);
int Que_IsInt(Que_State *state, int offset);
int Que_IsFloat(Que_State *state, int offset);
int Que_IsString(Que_State *state, int offset);
int Que_IsTable(Que_State *state, int offset);
int Que_IsFunction(Que_State *state, int offset);
int Que_IsCFunction(Que_State *state, int offset);

int Que_AsChar(Que_State *state, int offset, char *out_char);
int Que_AsInt(Que_State *state, int offset, Que_Int *out_int);
int Que_AsFloat(Que_State *state, int offset, Que_Float *out_float);
int Que_AsString(Que_State *state, int offset, char **out_str, size_t *out_length);

void Que_PushNil(Que_State *state);
void Que_PushChar(Que_State *state, char c);
void Que_PushChar(Que_State *state, char c);
void Que_PushBool(Que_State *state, int b);
void Que_PushInt(Que_State *state, Que_Int i);
void Que_PushFloat(Que_State *state, Que_Float f);
void Que_PushString(Que_State *state, const char *str);
void Que_PushCFunction(Que_State *state, Que_CFunction func);

void Que_SetGlobal(Que_State *state, int offset, const char *name);
int Que_GetGlobal(Que_State *state, const char *name);
Que_Value *Que_PopValue(Que_State *state);
void Que_LoadTable(Que_State *state, Que_TableObject *table, const char *name);

#endif /* QUE_STATE_H */
