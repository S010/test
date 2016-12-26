#CXX = clang++

CXXFLAGS += -std=c++11
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -Werror
CXXFLAGS += -pedantic

.PHONY: all clean

all: dns

clean:
	rm -f dns *.o

dns: dns.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
