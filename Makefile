CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS := -lasound -lpthread

TARGET := nassau_lane
SRCS := \
	src/main.cpp \
	src/AudioEngine.cpp \
	src/HardwareManager.cpp \
	src/Effects/Saturation.cpp \
	src/Effects/DoubleTrack.cpp \
	src/Effects/Tremolo.cpp \
	src/Effects/Reverb.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
