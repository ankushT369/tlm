# Compiler and target
CC      = gcc
TARGET  = tlm

# Source files (sqlite3.c and sqlite3.h are in the root now)
SRC     = main.c db.c ops.c tlm.c linenoise.c sqlite3.c
OBJ     = $(addprefix build/,$(SRC:.c=.o))

# Include paths (root for sqlite3.h)
INC     = -I .

# Libraries (adjust -lcurses to -lncurses if your system needs it)
LIBS    = -lm -lpthread -lcurses -ldl

# Flags
CFLAGS_DEBUG   = -g -O0 -Wall -Wextra
CFLAGS_RELEASE = -O3 -DNDEBUG

# Default rule
.PHONY: all debug release clean
all: release

# Build types
debug:   CFLAGS = $(CFLAGS_DEBUG)
release: CFLAGS = $(CFLAGS_RELEASE)

debug:   $(TARGET)
release: $(TARGET)

# Link
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

# Compile objects into build/ preserving directory structure
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Cleanup
clean:
	rm -rf build $(TARGET)
# CC = gcc
# TARGET = tlm
#
# SRC = main.c db.c ops.c tlm.c linenoise.c sqlite-src-3510300/sqlite3.c
# OBJ = $(addprefix build/,$(SRC:.c=.o))
#
# INC = -I sqlite-src-3510300
# LIBS = -lm -lpthread -lcurses -ldl
#
# CFLAGS_DEBUG = -g -O0 -Wall -Wextra
# CFLAGS_RELEASE = -O3 -DNDEBUG
#
# .PHONY: all debug release clean
#
# all: release
#
# debug: CFLAGS=$(CFLAGS_DEBUG)
# debug: $(TARGET)
#
# release: CFLAGS=$(CFLAGS_RELEASE)
# release: $(TARGET)
#
# $(TARGET): $(OBJ)
# 	$(CC) $(OBJ) -o $(TARGET) $(LIBS)
#
# build/%.o: %.c
# 	mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) $(INC) -c $< -o $@
#
# clean:
# 	rm -rf build $(TARGET)
