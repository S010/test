CXX = eg++
CXXFLAGS += -Wall
CXXFLAGS += -std=c++11
CXXFLAGS += -I/usr/local/include
LDFLAGS += -lgmpxx -lgmp

all: primes.o
	$(CXX) -o primes $(LDFLAGS) $<

clean:
	rm -f *.o primes
