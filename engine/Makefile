CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude -Ilib -MMD -MP
LDFLAGS := -B/usr/bin

SRCS := $(filter-out src/validate.cpp,$(wildcard src/*.cpp))
OBJS := $(SRCS:src/%.cpp=build/%.o)
DEPS := $(OBJS:.o=.d)

# vendored databento DBN parser (see engine/vendor/README.md for how it was built)
DBN_ROOT := vendor/databento
DBN_CXXFLAGS := -isystem $(DBN_ROOT)/include -I$(DBN_ROOT)/include/databento
DBN_LDFLAGS := -L$(DBN_ROOT)/lib -ldatabento $(HOME)/miniconda3/lib/libzstd.so -lz -lssl -lcrypto -lpthread

build/engine: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(DBN_LDFLAGS) -o $@

VALIDATE_OBJS := $(filter-out build/main.o,$(OBJS))

build/validate: src/validate.cpp $(VALIDATE_OBJS) | build
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(DBN_CXXFLAGS) $< $(VALIDATE_OBJS) $(DBN_LDFLAGS) -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(DBN_CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: clean

-include $(DEPS)
