CFLAGS = -Wall -Wextra -Werror -g3 -O0

all: wcat

wcat: wcat.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
