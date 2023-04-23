CC=gcc
CFLAGS=-Wall -Wextra

pcseml: pcseml.c eventbuf.c
	$(CC) $(CFLAGS) -o $@ $^