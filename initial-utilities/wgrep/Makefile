CFLAGS = -Wall -Wextra -Werror -g3 -O0

.PHONY: all
all: wgrep

wgrep: wgrep.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY:
clean:
	$(RM) wgrep *.o
