# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Source files
SRC = btree_mgr.c buffer_mgr.c buffer_mgr_stat.c cli.c dberror.c expr.c record_mgr.c rm_serializer.c storage_mgr.c
TEST_SRC = test_assign4_1.c

# Object files (each .c file has a corresponding .o file)
OBJ = $(SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

# Executables
EXEC = assignment_4

# Default target
all: $(EXEC)

# Rule for creating test_assign4_1 executable (if it includes main)
assignment_4: test_assign4_1.o $(filter-out cli.o, $(OBJ))
	$(CC) $(CFLAGS) -o $@ $^

# Compile each .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TEST_OBJ) $(EXEC)

# Phony targets
.PHONY: all clean