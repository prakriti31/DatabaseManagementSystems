# This MakeFile is manually written. To generate a more comprehensive MakeFile, use the command "cmake .".
# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
ASAN_FLAGS = -g -fsanitize=address

# Source files
SRCS = storage_mgr.c buffer_mgr.c buffer_mgr_stat.c test_assign2_1.c dberror.c
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = test_2_1

# Build rules
all: $(TARGET)

# Create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Compile each .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# AddressSanitizer build option
asan: CFLAGS += $(ASAN_FLAGS)
asan: clean $(TARGET)

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
