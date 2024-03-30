#include <stdio.h>
#include <que/state.h>
#include <stdlib.h>

#include "defs.h"

QUE_NORETURN void repl();

QUE_NORETURN void load(const char *path);

QUE_NORETURN void usage(const char *program);

int main(int argc, char *argv[]) {
        switch (argc) {
        case 1:
                repl();

        case 2:
                load(argv[1]);

        default:
                usage(argv[0]);
        }
}

void repl() {
        Que_State *state = Que_NewState();
        if (!state) {
                puts("[!] Failed to initialize. Now exiting.");
                exit(-10);
        }

        puts("Welcome to the Que interactive REPL!");
        for (;;) {
                char line[QUE_MAXLINE];

                fputs(">>> ", stdout);
                if (!fgets(line, sizeof(line), stdin)) {
                        puts("");
                        break;
                }

                if (Que_ExecuteString(state, line) != 0) {
                        puts("[!] Execution error");
                }
        }

        exit(-10);
}

void load(const char *path) {
        FILE *file = fopen(path, "rb");
        size_t size = 0;
        char *buf = NULL;
        Que_State *state = NULL;
        size_t bytes_read;

        if (!file) {
                fprintf(stderr, "[!] Failed to open '%s'.\n", path);
                goto cleanup;
        }

        fseek(file, 0L, SEEK_END);
        size = ftell(file);
        rewind(file);

        buf = malloc(size + 1);
        if (!buf) {
                fprintf(stderr, "[!] Out of memory.\n");
                goto cleanup;
        }

        bytes_read = fread(buf, 1, size, file);
        if (bytes_read < size) {
                fprintf(stderr, "[!] Could not read file '%s'\n", path);
                goto cleanup;
        }

        buf[bytes_read] = '\0';

        fclose(file);
        file = NULL;

        state = Que_NewState();
        if (!state) {
                fprintf(stderr, "[!] Out of memory.\n");
                goto cleanup;
        }

        exit(Que_ExecuteString(state, buf));

cleanup:
        if (file) {
                fclose(file);
        }

        if (buf) {
                free(buf);
        }

        if (state) {
                Que_DeleteState(state);
        }

        exit(75);
 }

void usage(const char *program) {
        fprintf(stderr, "Usage: %s [script] or just %s to launch REPL\n", program, program);
        exit(-10);
}
