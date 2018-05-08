CFLAGS ?= -Wall -Wextra
FIFO_NAME ?= scheduler_fifo.a
WSTEAL_NAME ?= scheduler_wsteal.a

COMMON_SRC := \
	scheduler.c

FIFO_SRC := \
	strategy_fifo.c

WSTEAL_SRC := \
	strategy_wsteal.c

SRCDIR := src
INCLUDEDIR := include
COMMON_OBJ := $(addprefix $(SRCDIR)/,$(COMMON_SRC:.c=.o))
FIFO_OBJ := $(addprefix $(SRCDIR)/,$(FIFO_SRC:.c=.o))
WSTEAL_OBJ := $(addprefix $(SRCDIR)/,$(WSTEAL_SRC:.c=.o))

CFLAGS += -MMD -MP -I$(INCLUDEDIR)

.PHONY: all clean fclean

all: $(FIFO_NAME) $(WSTEAL_NAME)

$(FIFO_NAME): $(COMMON_OBJ) $(FIFO_OBJ)

$(WSTEAL_NAME): $(COMMON_OBJ) $(WSTEAL_OBJ)

$(FIFO_NAME) $(WSTEAL_NAME):
	$(AR) rcs $@ $^

-include $(COMMON_OBJ:.o=.d)
-include $(FIFO_OBJ:.o=.d)
-include $(WSTEAL_OBJ:.o=.d)

clean:
	$(RM) $(COMMON_OBJ) $(COMMON_OBJ:.o=.d)
	$(RM) $(FIFO_OBJ) $(FIFO_OBJ:.o=.d)
	$(RM) $(WSTEAL_OBJ) $(WSTEAL_OBJ:.o=.d)

fclean:
	$(MAKE) clean
	$(RM) $(FIFO_NAME) $(WSTEAL_NAME)
