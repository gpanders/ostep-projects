CFLAGS = -Wall -Wextra -g3 -O0
OBJS = wzip.o

.PHONY: all
all: wzip

wzip: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(OBJS) wzip
