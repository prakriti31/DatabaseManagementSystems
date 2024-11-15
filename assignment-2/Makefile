CC = gcc
CFLAGS = -std=c11
# Uncomment the following line to enable AddressSanitizer
# CFLAGS += -g -fsanitize=address

SRCS = storage_mgr.c buffer_mgr.c buffer_mgr_stat.c dberror.c test_assign2_1.c
OBJS = $(SRCS:.c=.o)
TARGET = test_2_1

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
