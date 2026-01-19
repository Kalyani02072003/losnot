CC=gcc
CFLAGS=`pkg-config --cflags gtk+-3.0` -Wall -g
LIBS=`pkg-config --libs gtk+-3.0`

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

losnot: $(OBJ)
	$(CC) $(OBJ) -o losnot $(LIBS)

clean:
	rm -f src/*.o losnot
