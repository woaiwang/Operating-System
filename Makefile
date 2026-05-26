CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Iinclude
SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/fs

SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/block.c \
       $(SRCDIR)/inode.c \
       $(SRCDIR)/format.c \
       $(SRCDIR)/install.c \
       $(SRCDIR)/halt.c \
       $(SRCDIR)/name.c \
       $(SRCDIR)/access.c \
       $(SRCDIR)/dir.c \
       $(SRCDIR)/creat.c \
       $(SRCDIR)/open.c \
       $(SRCDIR)/close.c \
       $(SRCDIR)/rdwt.c \
       $(SRCDIR)/delete.c \
       $(SRCDIR)/log.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)