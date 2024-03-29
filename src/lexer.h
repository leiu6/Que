#ifndef QUE_LEXER_H
#define QUE_LEXER_H

#include <que/common.h>

#define TOKENS \
        X(TOK_OPEN_PAREN), X(TOK_CLOSE_PAREN),\
        X(TOK_OPEN_BRACKET), X(TOK_CLOSE_BRACKET),\
        X(TOK_INDENT), X(TOK_DEDENT),\
        \
        X(TOK_COMMA), X(TOK_COLON),\
        \
        X(TOK_PLUS), X(TOK_MINUS), X(TOK_STAR), X(TOK_STAR_STAR), X(TOK_SLASH),\
        X(TOK_GR), X(TOK_GREQ), X(TOK_LE), X(TOK_LEQ), X(TOK_LSHIFT), X(TOK_RSHIFT),\
        X(TOK_BAND), X(TOK_BOR), X(TOK_BXOR), X(TOK_AND), X(TOK_OR), X(TOK_NOT),\
        X(TOK_BNOT),\
        X(TOK_EQUAL), X(TOK_EQUAL_EQUAL), X(TOK_NOT_EQUAL),\
        \
        X(TOK_FUNCTION), X(TOK_LET), X(TOK_RETURN),\
        X(TOK_WHILE), X(TOK_BREAK), X(TOK_CONTINUE),\
        X(TOK_IF), X(TOK_ELSE),\
        X(TOK_NIL), X(TOK_TRUE), X(TOK_FALSE),\
        \
        X(TOK_IDENTIFIER),\
        X(TOK_CHAR), X(TOK_STRING), X(TOK_INT), X(TOK_FLOAT),\
        \
        X(TOK_EOL),\
        X(TOK_EOF),\
        X(TOK_ERROR)

typedef enum {
#define X(name) name
TOKENS
#undef X
} TokenType;

static const char *TOKEN_NAMES[] = {
#define X(name) #name
TOKENS
#undef X
};

typedef struct {
        TokenType type;
        const char *start;
        size_t length;
} Token;

void lexer_init(const char *source);

void lexer_next(Token *out_token);

size_t lexer_line(void);
size_t lexer_col(void);

#endif /* QUE_LEXER_H */
