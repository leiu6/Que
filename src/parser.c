#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "lexer.h"
#include "opcodes.h"
#include "value_internal.h"

#define MAX_LOCALS (QUE_BYTE_MAX + 1)

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
        int local_count;
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

static void error(const char *format, ...);

static void token_stringify(Que_Value *out_string, Token *token) {
        Que_ValueString(out_string, token->start, token->length);
}

static void init_compiler(Compiler *enclosing, Compiler *c, Token *identifier_token) {
        Local *local;
        Que_Value identifier;

        token_stringify(&identifier, identifier_token);

        c->enclosing = enclosing;

        c->func = allocate_function(&identifier);
        c->type = (c->enclosing) ? SCOPE_FUNCTION : SCOPE_SCRIPT;
        c->local_count = 0;
        c->scope_depth = 0;
        state.current_compiler = c;

        local = &state.current_compiler->locals[state.current_compiler->local_count++];
        local->depth = 0;
        local->name.start = "";
        local->name.length = 0;
}

static Chunk *current_chunk() {
        return &state.current_compiler->func->code;
}

static int identifiers_equal(Token *id1, Token *id2) {
        if (id1->length != id2->length) {
                return QUE_FALSE;
        }

        return memcmp(id1->start, id2->start, id1->length) == 0;
}

/**
 * Adds a local variable to the current scope
*/
static void add_local(Token *name) {
        Local *local = NULL;

        if (state.current_compiler->local_count == MAX_LOCALS) {
                error("Too many local variables in function");
                return;
        }

        local = &state.current_compiler->locals[state.current_compiler->local_count++];

        local->name = *name;
        local->depth = -1; /* Uninitialized */
}

static void mark_local_initialized() {
        state.current_compiler->locals[state.current_compiler->local_count - 1].depth =
                state.current_compiler->scope_depth;
}

/**
 * Determines whether we are able to create a local variable at the current scope
*/
static int does_it_exist(Token *name) {
        /* TODO: prevent shadowing */
        int i;
        for (i = state.current_compiler->local_count - 1; i >= 0; i--) {
                Local *local = &state.current_compiler->locals[i];

                if (local->depth != -1 && local->depth < state.current_compiler->scope_depth) {
                        break;
                }

                if (identifiers_equal(name, &local->name)) {
                        error("a variable already exists with the name %.*s in this scope", local->name.length, local->name.start);
                        return 1;
                }
        }

        return 0;
}

static int resolve_local(Token *name) {
        int i;
        for (i = state.current_compiler->local_count - 1; i >= 0; i--) {
                Local local = state.current_compiler->locals[i];

                if (identifiers_equal(&local.name, name)) { /* Find match? */
                        if (local.depth == -1) {
                                error("Cannot read uninitialized variable %.*s", local.name.length, local.name.start);
                        }

                        return i;
                }
        }

        return -1;
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

        /*
	while (state.current_compiler->local_count > 1 &&
	       state.current_compiler->locals[state.current_compiler->local_count - 1].depth >
	       state.current_compiler->scope_depth) {
		emit(OP_POP);
		state.current_compiler->local_count--;
	}
        */
}

static Que_FunctionObject *end_compiler() {
        Que_FunctionObject *result = state.current_compiler->func;

        emit(OP_RETURN);
        emit_word(result->arity);

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

static int peek(TokenType type);

void advance(void) {
        state.previous = state.current;

        for (;;) {
                lexer_next(&state.current);
                if (!peek(TOK_ERROR)) {
                        break;
                }

                error("%.*s", state.current.length, state.current.start);
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
}

int peek(TokenType type) {
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

void parse_expression(void);
void parse_primary(void);

void parse_table_access(void) {
        Token field = state.current;
        Que_Value v;
        consume(TOK_IDENTIFIER, "Expected identifier for table access");

        v.type = QUE_TYPE_STRING;
        v.value.o = (Que_Object *)allocate_string(field.start, field.length);

        emit(OP_PUSH);
        emit_constant(&v);

        emit(OP_TABLE_GET);
}

void parse_primary(void) {
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
		int slot = resolve_local(&identifier);

                if (match(TOK_EQUAL)) {
                        parse_expression();

                        if (slot == -1) {
                                Que_Value str;
                                token_stringify(&str, &identifier);

                                emit(OP_SET_GLOBAL);
                                emit_constant(&str);
                        } else {
                                emit(OP_SET_LOCAL);
                                emit_word(slot);
                        }
                } else {
                        if (slot == -1) {
                                Que_Value str;
                                token_stringify(&str, &identifier);

                                emit(OP_GET_GLOBAL);
                                emit_constant(&str);
                        } else {
                                emit(OP_GET_LOCAL);
                                emit_word(slot);
                }

                if (match(TOK_DOT)) {
                        parse_table_access();
                }
        }

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

        if (match(TOK_OPEN_PAREN)) {
                Que_Word argc = 0;

                if (!peek(TOK_CLOSE_PAREN)) {
                        for (;;) {
                                argc++;
                                parse_expression();
                                if (!match(TOK_COMMA)) {
                                        break;
                                }
                        }
                }

                consume(TOK_CLOSE_PAREN, "expected ')' after function call");

                /* Code */
                emit(OP_CALL);
                emit_word(argc);
        }
}

void parse_prefix(void) {
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
                parse_primary();
        }
}

void parse_multiply_divide(void) {
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

void parse_add_subtract(void) {
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

void parse_shift(void) {
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

void parse_comparison(void) {
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

void parse_bitwise_and_or_xor(void) {
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

void parse_and_or(void) {
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

void parse_expression(void) {
        parse_and_or();
}

static void parse_block();

void declare_variable(void) {

        consume(TOK_IDENTIFIER, "expected identifier for variable");

        if (state.current_compiler->type == SCOPE_FUNCTION) {
                /**
                 * If we are in a function, then this is a local variable
                 * 
                 * We:
                 * - check if it already exists, if so print error
                 * - if it does not exist, we add it to the locals table
                 * - we return, no instruction need to be emitted
                */
                if (!does_it_exist(&state.previous)) {
                        add_local(&state.previous);
                }
                return;
        }
}

void define_variable(Que_Value *identifier) {	
        parse_expression();

	if (state.current_compiler->type == SCOPE_SCRIPT) {
		emit(OP_DEFINE_GLOBAL);
		emit_constant(identifier);
	}
}

void parse_var_declaration() {
	Que_Value identifier;
	
        declare_variable();
	token_stringify(&identifier, &state.previous);

        if (match(TOK_EQUAL)) {
                define_variable(&identifier);
        } else {
                emit(OP_PUSH_NIL);
        }

        /**
         * If this is a local, we must mark it initialized now
        */
        if (state.current_compiler->type == SCOPE_FUNCTION) {
                mark_local_initialized();
        }

        consume(TOK_EOL, "expected newline");
}

void parse_declaration();

static int parse_function_args(void) {
        int arity = 0;

        consume(TOK_OPEN_PAREN, "expected '('");

        if (!peek(TOK_CLOSE_PAREN)) {
                do {
                        declare_variable();
                        mark_local_initialized();
                        arity++;
                } while (match(TOK_COMMA));
        }

        consume(TOK_CLOSE_PAREN, "expected ')'");

        return arity;
}

void parse_function_declaration() {
        Compiler compiler;
        Que_Value identifier;
        Que_Value function;

        consume(TOK_IDENTIFIER, "expected function identifier");
        token_stringify(&identifier, &state.previous);

        init_compiler(state.current_compiler, &compiler, &state.previous);
        begin_scope();

        compiler.func->arity = parse_function_args();

        consume(TOK_COLON, "expected ':'");
        consume(TOK_EOL, "expected '\\n'");
        consume(TOK_INDENT, "expected indent after function");
        
        while (!match(TOK_DEDENT) && !peek(TOK_EOF)) {
                parse_declaration();
        }
        
        end_scope();

        Que_ValueFunction(&function, end_compiler());

        emit(OP_PUSH);
        emit_constant(&function);
        emit(OP_DEFINE_GLOBAL);
        emit_constant(&identifier);
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
        parse_expression();
        consume(TOK_EOL, "expected newline after return");
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
        Token id;
        id.type = TOK_IDENTIFIER;
        id.start = "<script>";
        id.length = strlen("<script>");
        init_compiler(NULL, &c, &id);

        while (!match(TOK_EOF)) {
                parse_declaration();
        }

        return end_compiler();
}


