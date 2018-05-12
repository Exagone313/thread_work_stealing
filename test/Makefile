CFLAGS ?= -Wall -Wextra

TESTS := \
	quicksort

SERIAL := $(addsuffix .serial,$(TESTS))
LIFO := $(addsuffix .lifo,$(TESTS))
WSTEAL := $(addsuffix .wsteal,$(TESTS))

INCLUDEDIR := ../include

CFLAGS += -I$(INCLUDEDIR)
CFLAGS += -O3

.PHONY: all clean fclean

all: $(SERIAL) $(LIFO) $(WSTEAL)

%.serial: %.o ../scheduler_serial.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

%.lifo: %.o ../scheduler_lifo.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

%.wsteal: %.o ../scheduler_wsteal.a
	$(CC) $(CFLAGS) -o $@ -lpthread $^

clean:
	$(RM) $(addsuffix .o,$(TESTS))

fclean:
	$(MAKE) clean
	$(RM) $(SERIAL) $(LIFO) $(WSTEAL)
