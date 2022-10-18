CFLAGS = -Wall -Wextra -O2
OBJS = wunzip.o

.PHONY: all
all: wunzip

wunzip: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(OBJS) wunzip
