CFLAGS = -std=c99 -g -Wall -lpthread -O3
OBJFILES = ll.o queue.o gopher.o
CC = gcc

%.o:	%.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

PROJECT = gopher
doJobs:	$(OBJFILES)
	$(CC) -o $(PROJECT) $(CFLAGS) $(OBJFILES)

clean:
	rm $(OBJFILES) $(PROJECT)
