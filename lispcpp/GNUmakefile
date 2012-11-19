CXXFLAGS=-g -std=c++0x
CXX=g++

all: lisp

lisp: lisp.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $<

clean:
	rm -f *.o lisp
