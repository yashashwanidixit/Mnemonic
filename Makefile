# Makefile for IITPSx Assembler, Emulator, and Tests
# Name: Yashashwani | ID: 2501AI49
# "I declare this is my own work."

CXX = g++
CXXFLAGS = -std=c++17 -pedantic -Wall -Wextra -Wpointer-arith -Wwrite-strings -O2

all: asm emu

asm: asm.cpp
	$(CXX) $(CXXFLAGS) -o asm asm.cpp

emu: emu.cpp
	$(CXX) $(CXXFLAGS) -o emu emu.cpp

test: asm emu
	@bash run_tests.sh

clean:
	rm -f asm emu tests/*.o tests/*.lst tests/*.log

.PHONY: all test clean
