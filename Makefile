.PHONY: all production release clean

CC := gcc
BIN := m65xx

INCLUDES := -Iinclude/
STD := -std=c2x

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

# Default target 
all: production

# Production build 
production: CFLAGS := -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(STD) $(INCLUDES)
production: LDFLAGS := -ljansson -fsanitize=address,undefined
production: $(BIN)

# Release build
release: CFLAGS := -O3 -march=native -flto -fno-plt -fomit-frame-pointer -DNDEBUG $(STD) $(INCLUDES)
release: LDFLAGS := -ljansson -flto
release: $(BIN)

# Link the binary
$(BIN): $(OBJS)
	@echo "[linking]   $(OBJS)"
	@echo "[CFLAGS]    $(CFLAGS)"
	@echo "[LDFLAGS]   $(LDFLAGS)"
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)
	@echo "[produced]  $(BIN)"

# Compile each .c to .o
%.o: %.c
	@echo "[compiling] $<"
	@echo "[CFLAGS]    $(CFLAGS)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	@echo "[cleaned]   $(BIN) $(OBJS)"
	@rm -rvf $(OBJS) $(BIN) *.gch ncore.* > /dev/null 2>&1

