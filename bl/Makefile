CC=gcc
NAME=bl
CFLAGS=-Wall -g -c
OBJS=bl.o vlock.o barrier.o
all: $(NAME)

$(NAME): ${OBJS}
	$(CC) -o $@ ${OBJS}

bl.o: bl.c bl.h
	$(CC) ${CFLAGS} $< -o $@
	
vlock.o: vlock.c vlock.h
	$(CC) ${CFLAGS} $< -o $@

barrier.o: barrier.c barrier.h
	$(CC) ${CFLAGS} $< -o $@
	
clean:
	rm -f $(NAME) *.o
