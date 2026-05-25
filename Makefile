CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Iinclude
LDFLAGS :=

TARGET := build/fs.exe
SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)

all: $(TARGET)

build:
	@mkdir -p build

$(TARGET): build $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf build

.PHONY: all clean