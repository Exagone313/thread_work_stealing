CFLAGS ?= -Wall -Wextra
EXT ?=

TESTS := \
	quicksort

SERIAL := $(addsuffix .serial$(EXT),$(TESTS))
LIFO := $(addsuffix .lifo$(EXT),$(TESTS))
WSTEAL := $(addsuffix .wsteal$(EXT),$(TESTS))

INCLUDEDIR := ../include

CFLAGS += -I$(INCLUDEDIR)
CFLAGS += -O3

.PHONY: all clean fclean

all: $(SERIAL) $(LIFO) $(WSTEAL)

%.serial$(EXT): %.o ../scheduler_serial.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

%.lifo$(EXT): %.o ../scheduler_lifo.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

%.wsteal$(EXT): %.o ../scheduler_wsteal.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

clean:
	$(RM) $(addsuffix .o,$(TESTS))

fclean:
	$(MAKE) clean
	$(RM) $(SERIAL) $(LIFO) $(WSTEAL)
