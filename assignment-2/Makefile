CC = gcc
CFLAGS = -g -Wall -std=c11
INCLUDES = -I.

# Source files
SOURCES = storage_mgr.c buffer_mgr.c buffer_mgr_stat.c test_assign2_1.c dberror.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = assignment_2

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

# Debug target with address sanitizer
debug: CFLAGS += -fsanitize=address -DDEBUG
debug: clean all

# Dependencies
storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
buffer_mgr.o: buffer_mgr.c buffer_mgr.h buffer_mgr_stat.h storage_mgr.h dberror.h
buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
dberror.o: dberror.c dberror.h
test_assign2_1.o: test_assign2_1.c test_helper.h storage_mgr.h buffer_mgr.h dberror.h

.PHONY: all clean test debug