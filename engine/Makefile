CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude -Ilib -MMD -MP
LDFLAGS := -B/usr/bin

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=build/%.o)
DEPS := $(OBJS:.o=.d)

build/engine: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: clean

-include $(DEPS)
