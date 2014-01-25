EXE_NAME = ./dropblox_ai

CXXFLAGS += -std=c++0x -O3 -Wall -fopenmp

$(EXE_NAME): src/dropblox_ai.cpp src/choose_move.cpp
	clang++ $(CXXFLAGS) -o $@ $^

clean:
	rm $(EXE_NAME)
