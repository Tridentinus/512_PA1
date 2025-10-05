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

# Phony targets
.PHONY: all clean
