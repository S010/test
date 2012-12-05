CXXFLAGS += -g -Wall -std=c++11 -I/home/slava/src/misc/json
LDFLAGS += -L/home/slava/src/misc/json

all: server server2 test

server: server.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^ /home/slava/src/misc/json/{json,parse}.o

server2: server2.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^ /home/slava/src/misc/json/{json,parse}.o

test: test.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^ /home/slava/src/misc/json/{json,parse}.o

clean:
	rm -f *.o server server2 test
