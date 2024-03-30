#ifndef QUE_PARSER_H
#define QUE_PARSER_H

#include <que/value.h>

void parser_init(const char *filename, const char *source);

Que_FunctionObject *parser_parse();

#endif /* QUE_PARSER_H */
