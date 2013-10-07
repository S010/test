CXXFLAGS += -g -Wall -std=c++11 -I../../libeljson/include
LDFLAGS += -L../../libeljson/lib

all: server client

server: server.o
	$(CXX) $(LDFLAGS) -leljson -o $@ $<

client: client.o
	$(CXX) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.o server client
