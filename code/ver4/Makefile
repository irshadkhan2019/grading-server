CC = g++
CFLAGS = -pthread

all: server

server: main.o server.o
	$(CC) $(CFLAGS) -o server main.o server.o

main.o: main.cpp server.h
	$(CC) $(CFLAGS) -c main.cpp

server.o: server.cpp server.h
	$(CC) $(CFLAGS) -c server.cpp

clean:
	rm -f server *.o

