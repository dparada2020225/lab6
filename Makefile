CXX = g++
CXXFLAGS = -O2 -std=gnu++17 -Wall -Wextra -pthread
BIN = bin
SRC = $(wildcard src/*.cpp)
EXE = $(patsubst src/%.cpp,$(BIN)/%,$(SRC))

# Compilación con sanitizers
TSAN_FLAGS = -O1 -g -fsanitize=thread -fno-omit-frame-pointer -pthread
ASAN_FLAGS = -O1 -g -fsanitize=address -fno-omit-frame-pointer -pthread

.PHONY: all clean debug tsan asan

all: $(BIN) $(EXE)

$(BIN):
	mkdir -p $(BIN)

$(BIN)/%: src/%.cpp include/timing.hpp | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

# Versiones con sanitizers para debug
tsan: $(BIN)
	$(CXX) $(TSAN_FLAGS) src/p1_counter.cpp -o $(BIN)/p1_counter_tsan
	$(CXX) $(TSAN_FLAGS) src/p2_ring.cpp -o $(BIN)/p2_ring_tsan
	$(CXX) $(TSAN_FLAGS) src/p3_rw.cpp -o $(BIN)/p3_rw_tsan
	$(CXX) $(TSAN_FLAGS) src/p4_deadlock.cpp -o $(BIN)/p4_deadlock_tsan
	$(CXX) $(TSAN_FLAGS) src/p5_pipeline.cpp -o $(BIN)/p5_pipeline_tsan

asan: $(BIN)
	$(CXX) $(ASAN_FLAGS) src/p1_counter.cpp -o $(BIN)/p1_counter_asan
	$(CXX) $(ASAN_FLAGS) src/p2_ring.cpp -o $(BIN)/p2_ring_asan
	$(CXX) $(ASAN_FLAGS) src/p3_rw.cpp -o $(BIN)/p3_rw_asan
	$(CXX) $(ASAN_FLAGS) src/p4_deadlock.cpp -o $(BIN)/p4_deadlock_asan
	$(CXX) $(ASAN_FLAGS) src/p5_pipeline.cpp -o $(BIN)/p5_pipeline_asan

clean:
	rm -rf $(BIN)