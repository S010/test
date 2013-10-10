CXXFLAGS+=-std=c++11
CXXFLAGS+=-O2
LDFLAGS+=-lpthread

PROG=q

.PHONY: all clean

all: $(PROG)

clean:
		rm $(PROG) *.o

$(PROG): main.o
		$(CXX) $(LDFLAGS) -o $@ $<
