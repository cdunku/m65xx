.PHONY: all clean

CC := gcc
CFLAGS := -ggdb -O3 -Wall -Wextra -std=c2x -pedantic -Iinclude/ -fsanitize=address -ljansson
LDFLAGS := -ljansson `sdl-config --cflags --libs` -fsanitize=address
BIN := m65xx

# Find all .c files in the current directory
SRCS := $(wildcard src/*.c) 

# Convert .c files to .o files
OBJS := $(SRCS:.c=.o)

all: $(BIN)

# Link the object files to create the executable
$(BIN): $(OBJS)
	@echo "[linking] $(OBJS)"
	@echo "[produced] $(BIN)"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS)  

# Pattern rule to compile .c files into .o files with individual messages
%.o: %.c
	@echo "[compiling] $<"
	@$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	@echo "[cleaned] $(BIN) $(OBJS)"
	@rm -rvf $(OBJS) $(BIN) *.gch ncore.* > /dev/null 2>&1
