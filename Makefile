CC = gcc
OUT = vis
DIR_INC = include
DIR_SRC = src
DIR_OBJ = build
DIR_LIB = lib
DIR_LIB_SUB := $(wildcard $(DIR_LIB)/*)
DIR_LOG = logs
DIR_LOG_FILE = $(abspath $(DIR_LOG))/leak_check_$(shell date +"%Y%m%d_%H%M%S").log
DIR_SKD = examples

CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -fgnu89-inline -I $(DIR_INC) -isystem $(DIR_LIB)
LDLIBS = -lm -lGL -lGLEW -lX11 -lXrandr

SRC = $(wildcard $(DIR_SRC)/*.c)
OBJ = $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(SRC))

LIBS := $(foreach dir, $(DIR_LIB_SUB), $(shell $(MAKE) -C $(dir) --always-make | tail -n 2 | head -n 1 | grep -v "make: Nothing to be done for '[a-z]\+'"))
LDLIBS += $(foreach lib, $(LIBS), -l:$(lib))
LDFLAGS = $(foreach dir, $(DIR_LIB_SUB), -L$(abspath $(dir)))

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(DIR_OBJ):
	mkdir -p $(DIR_OBJ)

leak_check: clean
	$(MAKE) CFLAGS="$(CFLAGS) -ggdb3"
	-@echo $(DIR_LOG_FILE)
	valgrind --leak-check=full --show-reachable=yes --error-limit=no --log-file=$(DIR_LOG_FILE) $(abspath $(OUT)) $(wildcard $(DIR_SKD)/*.skd)

clean:
	rm -rf $(DIR_OBJ)
	$(foreach dir, $(DIR_LIB_SUB), $(MAKE) clean -C $(dir);)