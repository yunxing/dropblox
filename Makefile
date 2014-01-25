EXE_NAME = ./dropblox_ai

CXXFLAGS += -std=c++0x -O3 -Wall

$(EXE_NAME): C++/dropblox_ai.cpp
	clang++ $(CXXFLAGS) -o $@ $^

clean:
	rm $(EXE_NAME)
