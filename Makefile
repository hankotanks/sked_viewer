OUT = vis
DIR_INC = include
DIR_SRC = src
DIR_OBJ = build
DIR_LIB = lib

CFLAGS = -Wall -Wextra $(addprefix -isystem , $(shell find $(DIR_LIB) -type d)) -I$(DIR_INC)
# $(addprefix -I, $(shell find $(DIR_LIB) -type d))
LDLIBS = -lm -lGL -lGLEW -lX11 -lXrandr
LDFLAGS = $(addprefix -L, $(shell find $(DIR_LIB) -type d))

SRC = $(wildcard $(DIR_SRC)/*.c)
OBJ = $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(SRC))

# ensure sofa is linked properly
LIB_SOFA = $(DIR_LIB)/sofa/libsofa_c.a
LDLIBS += -l:$(notdir $(LIB_SOFA))

all: $(OUT)

$(OUT): $(OBJ)
	$(MAKE) $(LIB_SOFA)
	gcc $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	gcc $(CFLAGS) -c $< -o $@
$(DIR_OBJ):
	mkdir -p $(DIR_OBJ)

$(LIB_SOFA):
	$(MAKE) -C $(dir $(LIB_SOFA))
	$(MAKE) clean -C $(dir $(LIB_SOFA))

clean:
	rm -rf $(OUT) $(DIR_OBJ)
	$(MAKE) realclean -C $(dir $(LIB_SOFA))