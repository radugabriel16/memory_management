CC=gcc
CFLAGS=-Wall -Wextra -std=c99

TARGETS = sfl

build: $(TARGETS)

run sfl : sfl.c
	$(CC) $(CFLAGS) sfl.c -o sfl -lm

clean:
	rm -f $(TARGETS)

.PHONY: pack clean