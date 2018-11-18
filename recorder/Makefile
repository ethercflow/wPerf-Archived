src = $(wildcard *.c)
obj = $(src:.c=.o)

CC?=        cc
CFLAGS+=    -Wall -g -std=c11 -D_GNU_SOURCE
LDFLAGS =   -luv

.PHONY: all clean

all: recorder

recorder: $(obj)
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	@rm -f $(obj) recorder
