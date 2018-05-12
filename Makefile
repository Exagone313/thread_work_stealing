CFLAGS ?= -Wall -Wextra
LIFO_NAME ?= scheduler_lifo.a
WSTEAL_NAME ?= scheduler_wsteal.a
SERIAL_NAME ?= scheduler_serial.a

COMMON_SRC := \
	task.c

LIFO_SRC := \
	strategy_lifo.c

WSTEAL_SRC := \
	strategy_wsteal.c

SERIAL_SRC := \
	strategy_serial.c

SRCDIR := src
INCLUDEDIR := include
COMMON_OBJ := $(addprefix $(SRCDIR)/,$(COMMON_SRC:.c=.o))
LIFO_OBJ := $(addprefix $(SRCDIR)/,$(LIFO_SRC:.c=.o))
WSTEAL_OBJ := $(addprefix $(SRCDIR)/,$(WSTEAL_SRC:.c=.o))
SERIAL_OBJ := $(addprefix $(SRCDIR)/,$(SERIAL_SRC:.c=.o))

CFLAGS += -MMD -MP -I$(INCLUDEDIR)
CFLAGS += -O3

.PHONY: all clean fclean

all: $(LIFO_NAME) $(WSTEAL_NAME) $(SERIAL_NAME)

$(LIFO_NAME): $(COMMON_OBJ) $(LIFO_OBJ)

$(WSTEAL_NAME): $(COMMON_OBJ) $(WSTEAL_OBJ)

$(SERIAL_NAME): $(SERIAL_OBJ)

$(LIFO_NAME) $(WSTEAL_NAME) $(SERIAL_NAME):
	$(AR) rcs $@ $^

-include $(COMMON_OBJ:.o=.d)
-include $(LIFO_OBJ:.o=.d)
-include $(WSTEAL_OBJ:.o=.d)
-include $(SERIAL_OBJ:.o=.d)

clean:
	$(RM) $(COMMON_OBJ) $(COMMON_OBJ:.o=.d)
	$(RM) $(LIFO_OBJ) $(LIFO_OBJ:.o=.d)
	$(RM) $(WSTEAL_OBJ) $(WSTEAL_OBJ:.o=.d)
	$(RM) $(SERIAL_OBJ) $(SERIAL_OBJ:.o=.d)

fclean:
	$(MAKE) clean
	$(RM) $(LIFO_NAME) $(WSTEAL_NAME) $(SERIAL_NAME)
