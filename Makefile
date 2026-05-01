CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall
TARGET = dash
SRCS = dash.cpp lexer.cpp parser.cpp analyser.cpp reconciler.cpp interpreter.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
