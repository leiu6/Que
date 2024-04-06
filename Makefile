CFLAGS := -g -Wall -Werror -pedantic -std=c89 -fsanitize=address,undefined -Iinclude/ -DQUE_DEBUG_INSTRUCTIONS
LDFLAGS := -lm

SRCS := main.c lexer.c chunk.c memory.c state.c value.c vm.c table.c parser.c io.c
DEPS :=
OBJS := main.o lexer.o chunk.o memory.o state.o value.o vm.o table.o parser.o io.o

VPATH = src/ src/stdlib/ include/

.PHONY: all

all: que

que: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(OBJS) que

