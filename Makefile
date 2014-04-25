CC = g++
CFLAGS = -std=c++11
LDFLAGS = -lboost_system
OBJECTS = ddsn.o api_server.o local_peer.o code.o block.o

all: ddsn

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

ddsn: $(OBJECTS)
	$(CC) -o ddsn $(OBJECTS) $(LDFLAGS)
