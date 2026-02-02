CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I/opt/homebrew/opt/openssl/include
LDFLAGS = -L/opt/homebrew/opt/openssl/lib -lssl -lcrypto

SRC = main.c inode.c fs.c commands.c utils.c concu.c
OBJDIR = build
OBJ = $(patsubst %.c,$(OBJDIR)/%.o,$(SRC))
DEPS = pignoufs.h utils.h
TARGET = pignoufs

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
