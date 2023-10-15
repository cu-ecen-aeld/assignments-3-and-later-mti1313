
CFILES = *.c

ifndef CROSS_COMPILE
BINARY  = aesdsocket
CC      = gcc
CFLAGS  = -Wall -Werror
else
BINARY  = aesdsocket
CC      = $(CC)
CFLAGS  = -Wall -Werror
endif

all : $(BINARY)

$(BINARY) : $(CFILES)
		$(CC)  $(CFLAGS) -o $@ $^ $(LDFLAGS) 

.PHONY = clean
clean :
		rm -rf $(BINARY) $(BINARY).o
