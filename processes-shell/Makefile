CFLAGS = -Wall -Wextra -Wconversion

ifneq ($(DEBUG),)
	CFLAGS += -g3 -O0 -fsanitize=address -fsanitize=undefined
	LDFLAGS += -fsanitize=address -fsanitize=undefined
else
	CFLAGS += -DNDEBUG -O2
endif

OBJS = wish.o

.PHONY: all
all: wish

wish: wish.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(OBJS) wish
