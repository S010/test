CXXFLAGS += -Wall
CXXFLAGS += -Werror
CXXFLAGS += -Wextra
CXXFLAGS += -pedantic
CXXFLAGS += -std=c++11
CXXFLAGS += -O3

.PHONY: all clean test

PROGRAMS = selection insertion merge

all: $(PROGRAMS)

clean:
	rm -f *.o $(PROGRAMS)


test: $(PROGRAMS)
	@for prog in $^; do \
		echo $$prog; \
		echo '4 2 33 12 8 15 7 3' | ./$$prog; \
	done

