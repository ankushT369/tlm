CC = gcc
TARGET = tlm

SRC = main.c db.c ops.c tlm.c linenoise.c sqlite-src-3510300/sqlite3.c
OBJ = $(addprefix build/,$(SRC:.c=.o))

INC = -I sqlite-src-3510300
LIBS = -lm -lpthread -lcurses -ldl

CFLAGS_DEBUG = -g -O0 -Wall -Wextra
CFLAGS_RELEASE = -O3 -DNDEBUG

.PHONY: all debug release clean

all: release

debug: CFLAGS=$(CFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS=$(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf build $(TARGET)
