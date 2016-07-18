CFLAGS = -O3 -march=native -std=c99 -g -Wall -lpthread
OBJFILES = ll.o queue.o gopher.o
CC = gcc

%.o:	%.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

PROJECT = gopher
gopher:	$(OBJFILES)
	$(CC) -o $(PROJECT) $(CFLAGS) $(OBJFILES)

clean:
	rm $(OBJFILES) $(PROJECT)
