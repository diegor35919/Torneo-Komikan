CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -O2 -I include
LDFLAGS  = -pthread

SRCS_PART1 = src/main.cpp src/Config.cpp src/Matchmaker.cpp \
             src/Server.cpp src/Signals.cpp src/MockGame.cpp

TARGET = torneo

all: $(TARGET)

$(TARGET): $(SRCS_PART1)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_server: tests/test_server.cpp src/Config.cpp src/Matchmaker.cpp \
             src/Server.cpp src/Signals.cpp src/MockGame.cpp
	$(CXX) $(CXXFLAGS) $^ -o test_server $(LDFLAGS)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --error-exitcode=1 \
	         ./$(TARGET) 20 3

clean:
	rm -f $(TARGET) test_server

.PHONY: all clean valgrind test_server
