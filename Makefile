.PHONY: all clean

CC := gcc
CFLAGS := -O3 -std=c2x -Iinclude/ 
LDFLAGS := -ljansson -lncurses

#CC := o64-clang
#CFLAGS := -O3 -std=c2x -Iinclude/ -I$(HOME)/Github/osxcross/target/macports/pkgs/opt/local/include
#LDFLAGS := -L$(HOME)/Github/osxcross/target/macports/pkgs/opt/local/lib -ljansson -lncurses

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
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)  

# Pattern rule to compile .c files into .o files with individual messages
%.o: %.c
	@echo "[compiling] $<"
	@$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	@echo "[cleaned] $(BIN) $(OBJS)"
	@rm -rvf $(OBJS) $(BIN) *.gch ncore.* > /dev/null 2>&1
