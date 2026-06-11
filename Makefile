CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  ?=

TARGET   ?= app

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
