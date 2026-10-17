CXX = clang++
CPPFLAGS += $(shell pkg-config --cflags raylib)
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic
MODULE_FLAGS += -stdlib=libc++ -fmodule-file=std=std.pcm
LDLIBS += $(shell pkg-config --libs raylib)

.PHONY: all clean

all: chip8

chip8: main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(MODULE_FLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f chip8
