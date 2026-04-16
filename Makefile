CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lSDL2

SRC = $(shell find . -name "*.c")

OBJ = $(SRC:.c=.o)
OUT = chip8

all = $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

run: all
	./$(OUT)

clean:
	rm -f $(OUT) $(OBJ)
