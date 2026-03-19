# Compiler and target
CC      = gcc
TARGET  = tlm

# Source files
SRC     = main.c db.c ops.c tlm.c linenoise.c sqlite3.c
OBJ     = $(addprefix build/,$(SRC:.c=.o))

# Include paths
INC     = -I .

# Libraries
LIBS    = -lm -lpthread -lncurses -ldl

# Performance flags
CFLAGS_COMMON = -Wall -Wextra
CFLAGS_FAST   = -O3 -march=native -mtune=native -flto -funroll-loops -fomit-frame-pointer

# Optional (more aggressive, slightly unsafe)
# CFLAGS_FAST += -ffast-math

# SQLite performance tuning
SQLITE_FLAGS = \
    -DSQLITE_THREADSAFE=0 \
    -DSQLITE_DEFAULT_MEMSTATUS=0 \
    -DSQLITE_OMIT_LOAD_EXTENSION

# Link-time optimization
LDFLAGS = -flto

# Default
.PHONY: all clean
all: $(TARGET)

# Link
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS) $(LDFLAGS)

# Compile
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_FAST) $(INC) $(SQLITE_FLAGS) -c $< -o $@

# Clean
clean:
	rm -rf build $(TARGET)

# # Compiler and target
# CC      = gcc
# TARGET  = tlm
#
# # Source files (sqlite3.c and sqlite3.h are in the root now)
# SRC     = main.c db.c ops.c tlm.c linenoise.c sqlite3.c
# OBJ     = $(addprefix build/,$(SRC:.c=.o))
#
# # Include paths (root for sqlite3.h)
# INC     = -I .
#
# # Libraries (adjust -lcurses to -lncurses if your system needs it)
# LIBS    = -lm -lpthread -lcurses -ldl
#
# # Flags
# CFLAGS_DEBUG   = -g -O0 -Wall -Wextra
# CFLAGS_RELEASE = -Os -DNDEBUG -s
#
# # Default rule
# .PHONY: all debug release clean
# all: release
#
# # Build types
# debug:   CFLAGS = $(CFLAGS_DEBUG)
# release: CFLAGS = $(CFLAGS_RELEASE)
#
# debug:   $(TARGET)
# release: $(TARGET)
#
# # Link
# $(TARGET): $(OBJ)
# 	$(CC) $(OBJ) -o $(TARGET) $(LIBS)
#
# # Compile objects into build/ preserving directory structure
# build/%.o: %.c
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) $(INC) -c $< -o $@
#
# # Test
# test: tlm-test
# 	./tlm-test
# 	rm -f tlm-test
#
# tlm-test: tlm-test.c
# 	$(CC) $(CFLAGS_DEBUG) $^ -o tlm-test $(LIBS)
#
# # Cleanup
# clean:
# 	rm -rf build $(TARGET)
