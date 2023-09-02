CC := gcc

# Cross-compiler configuration
CROSS_COMPILE?=
SYSROOT :=

ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
    CC := $(CROSS_COMPILE)$(CC)
    SYSROOT := $(shell $(CC) -print-sysroot)
endif

# Compiler flags
CFLAGS := -Wall -Wextra

# Default target
default: writer

# Build the writer application
writer: writer.o
	$(CC) $(CFLAGS) -o $@ $^ $(SYSROOT)

# Compile writer.c to writer.o
writer.o: writer.c
	$(CC) $(CFLAGS) -c $< -o $@ --sysroot=$(SYSROOT)

# Clean target
clean:
	rm -f writer writer.o

.PHONY: default clean