#include <stdio.h>

#include "lexer.h"

int main(int argc, char *argv[]) {
        const char *source = "function\nlet\nreturn\nwhile\nbreak\ncontinue\nif\nelse\nnil\ntrue\nfalse\n__my_identifier";

        lexer_init(source);

        for (;;) {
                Token cur;
                size_t line, col;

                line = lexer_line();
                col = lexer_col();
                lexer_next(&cur);

                printf("%zu:%zu: [%s: ", line, col, TOKEN_NAMES[cur.type]);
                printf("%.*s]\n", (int)cur.length, cur.start);

                if (cur.type == TOK_ERROR ||
                    cur.type == TOK_EOF) {
                        break;
                }
        }

        return 0;
}
