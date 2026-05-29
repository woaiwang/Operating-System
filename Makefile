CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Iinclude
LDFLAGS :=

TARGET := build/fs.exe
SRCS := $(wildcard src/*.c) $(wildcard src/B/*.c) $(wildcard src/C/*.c)
# flatten: strip path prefix, keep only filename
OBJS := $(addprefix build/,$(notdir $(SRCS:.c=.o)))

all: $(TARGET)

build:
	@mkdir -p build

$(TARGET): build $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# src/*.c → build/*.o
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# src/B/*.c → build/*.o
build/%.o: src/B/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# src/C/*.c → build/*.o
build/%.o: src/C/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf build

.PHONY: all clean
