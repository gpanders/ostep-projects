CFLAGS = -Wall -Werror -g3 -O0 -fsanitize=address -fsanitize=undefined
LDFLAGS = -fsanitize=address -fsanitize=undefined
OBJS = kv.o

.PHONY: all
all: kv

kv: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(OBJS) kv
