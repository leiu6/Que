#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "lexer.h"
#include "opcodes.h"
#include "value_internal.h"

#define MAX_LOCALS 256

typedef struct {
        Token name;
        int depth;
} Local;

typedef enum {
        SCOPE_FUNCTION,
        SCOPE_SCRIPT
} ScopeType;

typedef struct Compiler Compiler;
struct Compiler {
        Compiler *enclosing;

        Que_FunctionObject *func;
        ScopeType type;

        Local locals[MAX_LOCALS];
        int num_locals;
        int scope_depth;
};


static struct {
        Token previous;
        Token current;

        Que_Byte had_error;
        Que_Byte panic_mode;
        ScopeType scope_type;

        const char *filename;

        Compiler *current_compiler;
} state;

static void init_compiler(Compiler *enclosing, Compiler *c, ScopeType type, int arity, const char *funcname, size_t len) {
        Local *local;

        c->enclosing = enclosing;

        c->func = allocate_function(arity, funcname, len);
        c->type = type;
        c->num_locals = 0;
        c->scope_depth = 0;
        state.current_compiler = c;

        local = &state.current_compiler->locals[state.current_compiler->num_locals++];
        local->depth = 0;
        local->name.start = "";
        local->name.length = 0;
}

static Chunk *current_chunk() {
        return &state.current_compiler->func->code;
}

static void emit(Que_Byte b) {
        chunk_write_byte(current_chunk(), b);
}

static void emit_word(Que_Word w);

static void emit_constant(Que_Value *v) {
        emit_word(
                chunk_write_constant(current_chunk(), v)
        );
}

void emit_word(Que_Word w) {
        chunk_write_word(current_chunk(), w);
}

void begin_scope() {
        state.current_compiler->scope_depth++;
}

void end_scope() {
        state.current_compiler->scope_depth--;
}

static Que_FunctionObject *end_compiler() {
        Que_FunctionObject *result = state.current_compiler->func;

        emit(OP_RETURN);

#ifdef QUE_DEBUG_INSTRUCTIONS
        printf("Function: %s\n", state.current_compiler->func->name->str);
        chunk_disassemble(current_chunk());
        puts("");
#endif

        if (state.current_compiler->enclosing) {
                state.current_compiler = state.current_compiler->enclosing;
        }

        return result;
}

static void advance(void);

static int match(TokenType type) {
        if (state.current.type == type) {
                advance();
                return QUE_TRUE;
        }
        return QUE_FALSE;
}

static void error(const char *format, ...);

void advance(void) {
        state.previous = state.current;
        lexer_next(&state.current);

        if (match(TOK_ERROR)) {
                error("%.*s", state.previous.length, state.previous.start);
        }
}

static void vferror(const char *format, va_list args);

void error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        vferror(format, args);
        va_end(args);
}

void vferror(const char *format, va_list args) {
        size_t line, col;

        if (state.had_error) {
                return;
        }

        state.had_error = QUE_TRUE;
        state.panic_mode = QUE_TRUE;

        line = lexer_line();
        col = lexer_col();

        fprintf(stderr, "%s:%zu:%zu: ", state.filename, line, col);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");

        advance();
}

static int peek(TokenType type) {
        return state.current.type == type;
}

static int consume(TokenType type, const char *format, ...) {
        if (!match(type)) {
                va_list args;
                va_start(args, format);
                vferror(format, args);
                va_end(args);
                return QUE_FALSE;
        }
        return QUE_TRUE;
}

void parser_init(const char *filename, const char *source) {
        lexer_init(source);
        lexer_next(&(state.current));
        state.had_error = state.panic_mode = QUE_FALSE;
        state.scope_type = SCOPE_SCRIPT;
        state.filename = filename;
        state.current_compiler = NULL;
}

void parse_expression();

void parse_primary() {
        if (match(TOK_INT)) {
                Que_Value v;
                v.type = QUE_TYPE_INT;
                v.value.i = strtol(state.previous.start, NULL, 10);
                emit(OP_PUSH);
                emit_constant(&v);
        } else if (match(TOK_FLOAT)) {
                Que_Value v;
                v.type = QUE_TYPE_FLOAT;
                v.value.f = strtod(state.previous.start, NULL);
                emit(OP_PUSH);
                emit_constant(&v);
        } else if (match(TOK_IDENTIFIER)) {
                Token identifier = state.previous;
                Que_Value str;
                str.type = QUE_TYPE_STRING;
                str.value.o = (Que_Object *)allocate_string(identifier.start, identifier.length);

                emit(OP_GET_GLOBAL);
                emit_constant(&str);
        } else if (match(TOK_STRING)) {
                Que_Value val;
                val.type = QUE_TYPE_STRING;
                val.value.o = (Que_Object *)allocate_string(state.previous.start, state.previous.length);
                emit(OP_PUSH);
                emit_constant(&val);
        } else if (match(TOK_CHAR)) {
                Que_Value val;
                val.type = QUE_TYPE_CHAR;
                val.value.c = state.previous.start[0];
                emit(OP_PUSH);
                emit_constant(&val);
        } else if (match(TOK_TRUE)) {
                emit(OP_PUSH_TRUE);
        } else if (match(TOK_FALSE)) {
                emit(OP_PUSH_FALSE);
        } else if (match(TOK_NIL)) {
                emit(OP_PUSH_NIL);
        } else if (match(TOK_OPEN_PAREN)) {
                parse_expression();
                consume(TOK_CLOSE_PAREN, "expected ')' after expression");
        } else {
                error("unexpected %.*s", state.current.length, state.current.start);
        }
}

void parse_access_call() {
        parse_primary();

        for (;;) {
                if (match(TOK_DOT)) {
                        Token field = state.current;
                        Que_Value v;
                        consume(TOK_IDENTIFIER, "expected identifier for table field");

                        v.type = QUE_TYPE_STRING;
                        v.value.o = (Que_Object *)allocate_string(field.start, field.length);

                        emit(OP_PUSH);
                        emit_constant(&v);

                        emit(OP_TABLE_GET);
                } else if (match(TOK_COLON)) {
                        Que_Word arity = 0;

                        /* Basically, we are pushing arguments to the stack */
                        for (;;) {
                                parse_expression();
                                arity++;
                                if (!match(TOK_COMMA)) {
                                        break;
                                }
                        }

                        emit(OP_CALL);
                        emit_word(arity);
                } else {
                        break;
                }
        } 
}

void parse_prefix() {
        if (match(TOK_NOT)) {
                parse_prefix();
                emit(OP_NOT);
        } else if (match(TOK_BNOT)) {
                parse_prefix();
                emit(OP_BNOT);
        } else if (match(TOK_MINUS)) {
                parse_prefix();
                emit(OP_NEGATE);
        } else {
                parse_access_call();
        }
}

void parse_multiply_divide() {
        parse_prefix();

        for (;;) {
                if (match(TOK_STAR)) {
                        parse_prefix();
                        emit(OP_MULTIPLY);
                } else if (match(TOK_SLASH)) {
                        parse_prefix();
                        emit(OP_DIVIDE);
                } else {
                        break;
                }
        }
}

void parse_add_subtract() {
        parse_multiply_divide();

        for (;;) {
                if (match(TOK_PLUS)) {
                        parse_multiply_divide();
                        emit(OP_ADD);
                } else if (match(TOK_MINUS)) {
                        parse_multiply_divide();
                        emit(OP_SUBTRACT);
                } else {
                        break;
                }
        }
}

void parse_shift() {
        parse_add_subtract();

        for (;;) {
                if (match(TOK_LSHIFT)) {
                        parse_add_subtract();
                        emit(OP_LSHIFT);
                } else if (match(TOK_RSHIFT)) {
                        parse_add_subtract();
                        emit(OP_RSHIFT);
                } else {
                        break;
                }
        }
}

void parse_comparison() {
        parse_shift();

        for (;;) {
                if (match(TOK_EQUAL_EQUAL)) {
                        parse_shift();
                        emit(OP_EQ);
                } else if (match(TOK_NOT_EQUAL)) {
                        parse_shift();
                        emit(OP_EQ);
                        emit(OP_NOT);
                } else if (match(TOK_GR)) {
                        parse_shift();
                        emit(OP_GR);
                } else if (match(TOK_GREQ)) {
                        parse_shift();
                        emit(OP_GREQ);
                } else if (match(TOK_LE)) {
                        parse_shift();
                        emit(OP_LE);
                } else if (match(TOK_LEQ)) {
                        parse_shift();
                        emit(OP_LEQ);
                } else {
                        break;
                }
        }
}

void parse_bitwise_and_or_xor() {
        parse_comparison();

        for (;;) {
                if (match(TOK_BAND)) {
                        parse_comparison();
                        emit(OP_BAND);
                } else if (match(TOK_BOR)) {
                        parse_comparison();
                        emit(OP_BOR);
                } else if (match(TOK_BXOR)) {
                        parse_comparison();
                        emit(OP_BXOR);
                } else {
                        break;
                }
        }
}

void parse_and_or() {
        parse_bitwise_and_or_xor();

        for (;;) {
                if (match(TOK_AND)) {
                        parse_bitwise_and_or_xor();
                        emit(OP_AND);
                } else if (match(TOK_OR)) {
                        parse_bitwise_and_or_xor();
                        emit(OP_OR);
                } else {
                        break;
                }
        }
}

void parse_expression() {
        parse_and_or();
}

static void parse_block();

void parse_var_declaration() {
        Token identifier = state.current;
        Que_Value str;

        consume(TOK_IDENTIFIER, "expected identifier for variable");
        
        if (match(TOK_EQUAL)) {
                parse_expression();
        } else {
                emit(OP_PUSH_NIL);
        }

        consume(TOK_EOL, "expected newline after variable declaration");

        emit(OP_SET_GLOBAL);

        str.type = QUE_TYPE_STRING;
        str.value.o = (Que_Object *)allocate_string(identifier.start, identifier.length);
        emit_constant(&str);
}

void parse_declaration();

void parse_function_declaration() {
        Compiler compiler;
        Token identifier = state.current;
        int arity = 0;
        Que_FunctionObject *func;
        Que_Value func_identifier_value;
        Que_Value function_value;
        function_value.type = QUE_TYPE_FUNCTION;

        printf("Identifier: %.*s\n", (int)identifier.length, identifier.start);

        consume(TOK_IDENTIFIER, "expected function identifier");
        init_compiler(state.current_compiler, &compiler, SCOPE_FUNCTION, 0, identifier.start, identifier.length);

        if (!peek(TOK_COLON)) {
                for (;;) {
                        consume(TOK_IDENTIFIER, "expected identifier for argument");
                        arity++;

                        if (match(TOK_COMMA)) {
                                continue;
                        } else {
                                break;
                        }
                }
        }

        compiler.func->arity = arity;

        consume(TOK_COLON, "expected ':'");
        consume(TOK_EOL, "expected '\\n'");
        consume(TOK_INDENT, "expected indent after function");

        begin_scope();
        
        while (!match(TOK_DEDENT)) {
                parse_declaration();
        }
        
        end_scope();

        func = end_compiler();
        function_value.value.o = (Que_Object *)func;

        emit(OP_PUSH);
        emit_constant(&function_value);
        emit(OP_SET_GLOBAL);
        func_identifier_value.type = QUE_TYPE_STRING;
        func_identifier_value.value.o = (Que_Object *)func->name;
        emit_constant(&func_identifier_value);
}

void parse_if_statement() {
        assert(0 && "not implemented");
}

void parse_while_statement() {
        assert(0 && "not implemented");
}

void parse_declaration();

void parse_block() {
        while (!match(TOK_DEDENT) && !peek(TOK_EOF)) {
                parse_declaration();
        }
}

void parse_expression_statement() {
        parse_expression();
        consume(TOK_EOL, "expected newline after expression");

        emit(OP_POP);
}

void parse_return_statement() {
        assert(0 && "not implemented");
}

void parse_statement() {
        if (match(TOK_WHILE)) {
                parse_while_statement();
        } else if (match(TOK_IF)) {
                parse_if_statement();
        } else if (match(TOK_RETURN)) {
                parse_return_statement();
        } else if (match(TOK_WHILE)) {
                parse_while_statement();
        } else if (match(TOK_INDENT)) {
                begin_scope();
                parse_block();
                end_scope();
        } else if (match(TOK_EOL)) {
                return;
        } else {
                parse_expression_statement();
        }
}

void parse_declaration() {
        if (match(TOK_LET)) {
                parse_var_declaration();
        } else if (match(TOK_FUNCTION)) {
                parse_function_declaration();
        } else {
                parse_statement();
        }
}

Que_FunctionObject *parser_parse() {
        Compiler c;
        init_compiler(NULL, &c, SCOPE_SCRIPT, 0, state.filename, strlen(state.filename));

        while (!match(TOK_EOF)) {
                parse_declaration();
        }

        return end_compiler();
}


