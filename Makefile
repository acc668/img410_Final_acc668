CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++11
LDFLAGS = -lm


SHARED_SRCS = ppm.cpp


RAYCAST_SRCS = raycast.cpp $(SHARED_SRCS)
RAYCAST_BIN = raycast


ANIMATE_SRCS = animateMain.cpp animate.cpp $(SHARED_SRCS)
ANIMATE_BIN = animate

.PHONY: all clean

all: $(RAYCAST_BIN) $(ANIMATE_BIN)

$(RAYCAST_BIN): $(RAYCAST_SRCS) raycast.hpp ppm.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(RAYCAST_SRCS) $(LDFLAGS)

$(ANIMATE_BIN): $(ANIMATE_SRCS) animate.hpp raycast.hpp ppm.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(ANIMATE_SRCS) $(LDFLAGS)

clean:
	rm -f $(RAYCAST_BIN) $(ANIMATE_BIN)