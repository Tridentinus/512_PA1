# Compiler
CXX = g++

# Default build mode (can override with `make MODE=debug`)
MODE ?= release

# Flags for each mode
ifeq ($(MODE),debug)
    CXXFLAGS = -std=c++11 -Wall -Wextra -g
else ifeq ($(MODE),release)
    CXXFLAGS = -std=c++11 -Wall -Wextra -O3
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
	rm -f $(OBJS) $(TARGET)

# Run target: build and run the program with configurable parameters
# Usage examples:
#   make run TIME=30 FAKE=true NAME=3    # uses fake_inv.param/fake_wire.param and examples/3.txt
#   make run TIME=30 FAKE=false NAME=3   # uses inv.param/wire.param and examples/3.txt
# Defaults: TIME=30, FAKE=true, NAME=3
TIME ?= 30
FAKE ?= true
NAME ?= 3

run: all
	@echo "Running $(TARGET) with TIME=$(TIME), FAKE=$(FAKE), NAME=$(NAME)"
	@if [ "$(FAKE)" = "true" ]; then \
		INV=fake_inv.param; WIRE=fake_wire.param; \
	else \
		INV=inv.param; WIRE=wire.param; \
	fi; \
	INPUT=examples/$(NAME).txt; \
	OUT_PRE=out/$(NAME).pre; \
	OUT_ELM=out/$(NAME).elmore; \
	OUT_TTOPO=out/$(NAME).ttopo; \
	OUT_BTOPO=out/$(NAME).btopo; \
	mkdir -p out; \
	./$(TARGET) $(TIME) $$INV $$WIRE $$INPUT $$OUT_PRE $$OUT_ELM $$OUT_TTOPO $$OUT_BTOPO

# Phony targets
.PHONY: all clean
