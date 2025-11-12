# Compiler
CXX = g++

# Default build mode (can override with `make MODE=debug`)
MODE ?= release
LOGG ?= false # if true -DDEBUG is defined


# Flags for each mode
ifeq ($(MODE),release)
	CXXFLAGS = -std=c++11 -pedantic -Wvla -Wall -Wshadow -O3
else ifeq ($(LOGG),true)
	CXXFLAGS = -std=c++11 -pedantic -Wvla -Wall -Wshadow -g -DDEBUG
else
	CXXFLAGS = -std=c++11 -pedantic -Wvla -Wall -Wshadow -g
endif



# Target executable
TARGET = pa1

# Source and object files
SRCS = main.cpp tree.cpp node.cpp
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET) ./out/*

# Run target: build and run the program with configurable parameters
# Usage examples:
#   make run TIME=30 FAKE=true NAME=3    # uses fake_inv.param/fake_wire.param and examples/3.txt
#   make run TIME=30 FAKE=false NAME=3   # uses inv.param/wire.param and examples/3.txt
# Defaults: TIME=30, FAKE=true, NAME=3
TIME ?= 20
FAKE ?= true
NAME ?= 3

# Compute file paths (make-level variables - avoids shell scoping issues)
INV := $(if $(filter true,$(FAKE)),fake_inv.param,inv.param)
WIRE := $(if $(filter true,$(FAKE)),fake_wire.param,wire.param)
INPUT := examples/$(NAME).txt
OUT_PRE := out/$(NAME).pre
OUT_ELM := out/$(NAME).elmore
OUT_TTOPO := out/$(NAME).ttopo
OUT_BTOPO := out/$(NAME).btopo
LOG := out/$(NAME).valgrind.log
CALL := out/$(NAME).callgrind.out

run: all
	@echo "Running $(TARGET) with TIME=$(TIME), FAKE=$(FAKE), NAME=$(NAME)"
	mkdir -p out
	./$(TARGET) $(TIME) $(INV) $(WIRE) $(INPUT) $(OUT_PRE) $(OUT_ELM) $(OUT_TTOPO) $(OUT_BTOPO)

.PHONY: run-valgrind
run-valgrind: all
	@echo "Running $(TARGET) under valgrind with TIME=$(TIME), FAKE=$(FAKE), NAME=$(NAME)"
	mkdir -p out
	# Run memcheck
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=$(LOG) \
		./$(TARGET) $(TIME) $(INV) $(WIRE) $(INPUT) $(OUT_PRE) $(OUT_ELM) $(OUT_TTOPO) $(OUT_BTOPO)
# 	# Run callgrind for profiling (optional)
# 	valgrind --tool=callgrind --callgrind-out-file=$(CALL) ./$(TARGET) $(TIME) $(INV) $(WIRE) $(INPUT) $(OUT_PRE) $(OUT_ELM) $(OUT_TTOPO) $(OUT_BTOPO)
	@echo "Valgrind memcheck log: $(LOG)"
# 	@echo "Callgrind output: $(CALL)"

run-gdb: all
	@echo "Running $(TARGET) under gdb with TIME=$(TIME), FAKE=$(FAKE), NAME=$(NAME)"
	mkdir -p out
	gdb --args ./$(TARGET) $(TIME) $(INV) $(WIRE) $(INPUT) $(OUT_PRE) $(OUT_ELM) $(OUT_TTOPO) $(OUT_BTOPO)


# Phony targets
.PHONY: all clean
