CC = gcc
CFLAGS = -Wall

# Find all .c files in the current directory
SOURCES := $(wildcard *.c)

# Generate a list of corresponding executable names
PROGRAMS := $(patsubst %.c,%,$(SOURCES))

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS)
