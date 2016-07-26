LIBS=-lm
CFLAGS=-Wall
CC=gcc

SRCDIR=src
BINDIR=bin
SRC=$(wildcard $(SRCDIR)/*.c)
BIN=$(patsubst $(SRCDIR)/%.c,%,$(SRC))

all:$(BIN)

%: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $< 
