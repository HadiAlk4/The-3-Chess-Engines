CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
SRC := $(wildcard src/*.cpp)

engine: $(SRC) $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -Isrc $(SRC) -o engine

perft: engine
	./engine perft

clean:
	rm -f engine

.PHONY: perft clean
