CC=gcc
NAME=hotspot
CFLAGS=-Wall -g -c
OBJS=hotspot.o barrier.o
all: $(NAME)

$(NAME): ${OBJS}
	$(CC) -o $@ ${OBJS}

hotspot.o: hotspot.c hotspot.h
	$(CC) ${CFLAGS} $< -o $@

barrier.o: barrier.c barrier.h
	$(CC) ${CFLAGS} $< -o $@

clean:
	rm -f $(NAME) *.o
