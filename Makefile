CC = g++
CFLAGS = -std=c++11
LDFLAGS = -lboost_system -lboost_program_options -lcrypto
OBJECTS = ddsn.o api_server.o api_connection.o api_messages.o peer_server.o peer_connection.o peer_messages.o peer_id.o local_peer.o code.o block.o

all: ddsn

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

ddsn: $(OBJECTS)
	$(CC) -o ddsn $(OBJECTS) $(LDFLAGS)

clean:
	rm -f *.o ddsn
