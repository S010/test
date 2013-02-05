CXXFLAGS += -g -Wall -std=c++11 -I/home/slava/src/misc/json
LDFLAGS += -L/home/slava/src/misc/json

all: server client

server: server.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^ /home/slava/src/misc/json/{json,parse}.o

client: client.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

clean:
	rm -f *.o server client
