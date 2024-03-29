CC := clang

CFLAGS := -Wall -Werror -pedantic -std=c90 -fsanitize=address,undefined -Iinclude/
LDFLAGS := -lm

SRCS := main.c lexer.c
DEPS :=
OBJS := main.o lexer.o

VPATH = src/ include/

.PHONY: all

all: que

que: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(OBJS) que

