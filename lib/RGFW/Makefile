CC = gcc
AR = ar
RGFW_LIB_NAME = libRGFW.a

all: $(RGFW_LIB_NAME)
	@echo $(RGFW_LIB_NAME)

$(RGFW_LIB_NAME): RGFW.o
	$(AR) rcs $(RGFW_LIB_NAME) RGFW.o

RGFW.o: RGFW.c RGFW.h
	$(CC) -O2 -c RGFW.c -o RGFW.o 2>/dev/null

clean:
	rm -f RGFW.o $(RGFW_LIB_NAME)
