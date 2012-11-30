CXXFLAGS += -g -Wall -std=c++11

all: server

server: server.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

clean:
	rm -f *.o server
