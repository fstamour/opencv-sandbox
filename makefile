
CXX = g++
CXXFLAGS = -std=c++14

all: main

main: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o main  $(shell pkg-config --cflags --libs opencv)
