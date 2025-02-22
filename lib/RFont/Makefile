CC = gcc
AR = ar
RFONT_LIB_NAME = libRFont.a

all: $(RFONT_LIB_NAME)

$(RFONT_LIB_NAME): RFont.o
	$(AR) rcs $(RFONT_LIB_NAME) RFont.o 2>/dev/null
	@echo $(RFONT_LIB_NAME)

RFont.o: RFont.c RFont.h
	$(CC) -O2 -c RFont.c -o RFont.o 2>/dev/null

clean:
	rm -f RFont.o $(RFONT_LIB_NAME)
