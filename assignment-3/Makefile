CC = gcc
CFLAGS = -g -Wall -std=c11
INCLUDES = -I.

# Source files for both executables
COMMON_SRCS = buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c rm_serializer.c storage_mgr.c record_mgr.c
TEST_SRCS = test_assign3_1.c $(COMMON_SRCS)
CLI_SRCS = cli.c $(COMMON_SRCS)

# Object files
TEST_OBJS = $(TEST_SRCS:.c=.o)
CLI_OBJS = $(CLI_SRCS:.c=.o)

# Executables
TEST_EXEC = test_assign3_1
CLI_EXEC = cli

# Default target
all: $(TEST_EXEC) $(CLI_EXEC)

# Linking rules
$(TEST_EXEC): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(CLI_EXEC): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation rule
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(TEST_OBJS) $(CLI_OBJS) $(TEST_EXEC) $(CLI_EXEC)

# Run test target
test: $(TEST_EXEC)
	./$(TEST_EXEC)

# Dependencies
buffer_mgr.o: buffer_mgr.c buffer_mgr.h buffer_mgr_stat.h storage_mgr.h dberror.h
buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
dberror.o: dberror.c dberror.h
expr.o: expr.c expr.h dberror.h record_mgr.h tables.h
record_mgr.o: record_mgr.c record_mgr.h buffer_mgr.h storage_mgr.h
rm_serializer.o: rm_serializer.c record_mgr.h
storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
test_assign3_1.o: test_assign3_1.c test_helper.h storage_mgr.h buffer_mgr.h record_mgr.h
cli.o: cli.c storage_mgr.h buffer_mgr.h record_mgr.h

.PHONY: all clean test