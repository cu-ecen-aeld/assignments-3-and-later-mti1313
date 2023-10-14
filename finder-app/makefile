CC = $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)
CFLAGS = -Wall -Wextra

# Target application
TARGET = writer

# Source files
SRCS = writer.c

# Object files
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

# Default target to build the application
all: $(TARGET)

# Rule to build the application
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target to remove application and object files
clean:
	rm -f $(TARGET) $(OBJS)
