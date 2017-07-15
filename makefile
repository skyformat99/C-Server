CC = gcc
FILE = sws.c
EXEC = sws

build: $(FILE)
		$(CC) -o $(EXEC) $(FILE)

clean:
		rm -f $(EXEC) *.o