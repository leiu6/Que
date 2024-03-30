#include "lexer.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "defs.h"

static struct {
        const char *start;
        const char *current;
        size_t line;
        size_t col;

        Que_Byte at_line_begin;
        Que_Byte indent_levels[QUE_MAX_INDENT];
        Que_Byte *current_indent_level;
        Que_Byte dedent_emit_count;
} state;

void lexer_init(const char *source) {
        state.start = state.current = source;
        state.line = 1;
        state.col = 0;

        state.at_line_begin = QUE_TRUE;
        state.indent_levels[0] = 0;
        state.current_indent_level = state.indent_levels;
        state.dedent_emit_count = 0;
}

static char peek() {
        return *state.current;
}

static char advance() {
        state.at_line_begin = QUE_FALSE;
        state.col++;
        return (*state.current == '\0') ? '\0' : *state.current++;
}

static int match(char expected) {
        if (peek() == expected) {
                advance();
                return QUE_TRUE;
        }
        return QUE_FALSE;
}

static void token_simple(Token *out_token, TokenType type) {
        out_token->type = type;
        out_token->start = state.start;
        out_token->length = state.current - state.start;
}

static void token_error(Token *out_token, const char *msg) {
        out_token->type = TOK_ERROR;
        out_token->start = msg;
        out_token->length = strlen(msg);
}

#define IS_MULTIPLE_OF(num, of) \
        ((num % of) == 0)

#define GET_REMAINDER(num, of) \
        (num % of)

static int token_indentation(Token *out_token) {
        Que_Word spaces = 0;
        Que_Byte indent_level;

        if (!state.at_line_begin) {
                return QUE_FALSE;
        }

        while (peek() == ' ') {
                advance();
                spaces++;
        }

        if (!IS_MULTIPLE_OF(spaces, QUE_INDENT_WIDTH)) {
                token_error(out_token, "invalid number of spaces for indent");
                return QUE_TRUE;
        }

        indent_level = spaces / QUE_INDENT_WIDTH;

        if (indent_level > *state.current_indent_level) {
                token_simple(out_token, TOK_INDENT);
                *(++state.current_indent_level) = indent_level;
                return QUE_TRUE;
        } else if (indent_level < *state.current_indent_level) {
                Que_Byte dedents = 0;

                while (indent_level < *state.current_indent_level) {
                        dedents++;
                        state.current_indent_level--;
                }

                state.dedent_emit_count = dedents - 1;
                token_simple(out_token, TOK_DEDENT);

                return QUE_TRUE;
        } else {
                return QUE_FALSE;
        }
}

static void token_eol(Token *out_token) {
        state.at_line_begin = QUE_TRUE;
        state.line++;
        state.col = 0;

        token_simple(out_token, TOK_EOL);
}

static void token_string(Token *out_token) {
        for (;;) {
                char c = advance();

                if (c == '\\') {
                        advance();
                        continue;
                } else if (c == '"') {
                        break;
                }
        }

        out_token->type = TOK_STRING;
        out_token->start = state.start + 1;
        out_token->length = state.current - state.start - 2;
}

static void token_char(Token *out_token) {
        advance();
        if (!match('\'')) {
                token_error(out_token, "expected ' after char");
                return;
        }

        out_token->type = TOK_CHAR;
        out_token->start = state.start + 1;
        out_token->length = state.current - state.start - 2;
}

static void token_number(Token *out_token) {
        while (isnumber(peek())) {
                advance();
        }

        if (!match('.')) {
                out_token->type = TOK_INT;
                out_token->start = state.start;
                out_token->length = state.current - state.start;
                return;
        }

        while(isnumber(peek())) {
                advance();
        }

        out_token->type = TOK_FLOAT;
        out_token->start = state.start;
        out_token->length = state.current - state.start;
}

static TokenType check_keyword(size_t start, size_t length, const char *rest, TokenType type) {
        if (state.current - state.start == start + length &&
            memcmp(state.start + start, rest, length) == 0) {
                return type;
        }
        return TOK_IDENTIFIER;
}

static TokenType identifier_keyword_type() {
        switch (state.start[0]) {
        case 'b': return check_keyword(1, 4, "reak", TOK_BREAK);
        case 'c': return check_keyword(1, 7, "ontinue", TOK_CONTINUE);
        case 'e': return check_keyword(1, 3, "lse", TOK_ELSE);
        case 'f': {
                if (state.current - state.start > 1) {
                        switch (state.start[1]) {
                        case 'a': return check_keyword(2, 3, "lse", TOK_FALSE);
                        case 'u': return check_keyword(2, 6, "nction", TOK_FUNCTION);
                        }
                }
        } break;
        case 'i': return check_keyword(1, 1, "f", TOK_IF);
        case 'l': return check_keyword(1, 2, "et", TOK_LET);
        case 'n': return check_keyword(1, 2, "il", TOK_NIL);
        case 'r': return check_keyword(1, 5, "eturn", TOK_RETURN);
        case 't': return check_keyword(1, 3, "rue", TOK_TRUE);
        case 'w': return check_keyword(1, 4, "hile", TOK_WHILE);
        }

        return TOK_IDENTIFIER;
}

static void token_identifier(Token *out_token) {
        while (isalnum(peek()) || peek() == '_') {
                advance();
        }

        token_simple(out_token, identifier_keyword_type());
}

void lexer_next(Token *out_token) {
        char c;

        /* Emit queued dedents */
        if (state.dedent_emit_count > 0) {
                token_simple(out_token, TOK_DEDENT);
                state.dedent_emit_count--;
                return;
        }

        /* Handle indentation */
        if (state.at_line_begin) {
                if (token_indentation(out_token)) {
                        state.at_line_begin = QUE_FALSE;
                        return;
                }
        }

        c = peek();
        do {
                state.start = state.current;
                c = advance();
        } while (c == ' ');

        switch (c) {
        case '(': token_simple(out_token, TOK_OPEN_PAREN); return;
        case ')': token_simple(out_token, TOK_CLOSE_PAREN); return;
        case '[': token_simple(out_token, TOK_OPEN_BRACKET); return;
        case ']': token_simple(out_token, TOK_CLOSE_BRACKET); return;
        case ',': token_simple(out_token, TOK_COMMA); return;
        case '.': token_simple(out_token, TOK_DOT); return;
        case ':': token_simple(out_token, TOK_COLON); return;
        case '+': token_simple(out_token, TOK_PLUS); return;
        case '-': token_simple(out_token, TOK_MINUS); return;
        case '*': token_simple(out_token,
                match('*') ? TOK_STAR_STAR : TOK_STAR
        ); return;
        case '/': token_simple(out_token, TOK_SLASH); return;
        /* I'm sorry for the three-way ternary */
        case '>': token_simple(out_token,
                match('>') ? TOK_RSHIFT : (
                        match('=') ? TOK_GREQ : TOK_GR
                )
        ); return;
        case '<': token_simple(out_token,
                match('<') ? TOK_LSHIFT : (
                        match('=') ? TOK_LEQ : TOK_LE
                )
        ); return;
        case '&': token_simple(out_token,
                match('&') ? TOK_AND : TOK_BAND
        ); return;
        case '|': token_simple(out_token,
                match('|') ? TOK_OR : TOK_BOR
        ); return;
        case '^': token_simple(out_token, TOK_BXOR); return;
        case '~': token_simple(out_token, TOK_BNOT); return;
        case '!': token_simple(out_token,
                match('=') ? TOK_NOT_EQUAL : TOK_NOT
        ); return;
        case '=': token_simple(out_token,
                match('=') ? TOK_EQUAL_EQUAL : TOK_EQUAL
        ); return;
        case '\0': token_simple(out_token, TOK_EOF); return;
        case '\n': token_eol(out_token); return;
        case '"': token_string(out_token); return;
        case '\'': token_char(out_token); return;

        default: {
                if (isnumber(c)) {
                        token_number(out_token);
                        return;
                } else if (isalpha(c) || c == '_') {
                        token_identifier(out_token);
                        return;
                }

                token_simple(out_token, TOK_ERROR);
                return;
        }
        
        }
}

size_t lexer_line(void) {
        return state.line;
}

size_t lexer_col(void) {
        return state.col;
}
