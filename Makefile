CXX=g++
CXXFLAGS=-std=c++23 -Wall -Wextra -g
BIN=sleep_server

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^

%.o: %.c
	$(CXX) $@ -c $<

clean:
	rm -f *.o $(BIN)
