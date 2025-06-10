CC := gcc
OUT := vis

all: $(OUT)

DIR_INC := include
DIR_SRC := src
DIR_OBJ := build

CFLAGS := -Wall -Wextra -Wconversion -Wpedantic -I$(DIR_INC) -DLOGGING -DDISABLE_OVERLAY_UI
	
$(OUT): $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
# build dependencies
	$(MAKE) -s -C glenv
	$(MAKE) -s -C sofa
# build target executable
	$(CC) $(CFLAGS) $^ -o $@ $(shell $(MAKE) get_bin_flags -s -C glenv) -isystemsofa -Lsofa -lsofa_c

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(shell $(MAKE) get_obj_flags -s -C glenv) -isystemsofa

clean:
	$(RM) -r $(DIR_OBJ)/*.o
	$(MAKE) clean -s -C sofa
	$(MAKE) clean -s -C glenv

.PHONY: all clean