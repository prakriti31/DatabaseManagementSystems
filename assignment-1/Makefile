CC = gcc
CFLAGS = -g -Wall -std=c17
INCLUDES = -I.

# Source files
SOURCES = storage_mgr.c dberror.c test_assign1_1.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = assignment_1

# Default target
all: $(TARGET)

# Linking rule
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation rule
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run test target
test: $(TARGET)
	./$(TARGET)

# Debug target
debug: CFLAGS += -DDEBUG
debug: clean all

# Dependencies
storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
dberror.o: dberror.c dberror.h
test_assign1_1.o: test_assign1_1.c test_helper.h storage_mgr.h dberror.h

.PHONY: all clean test debug