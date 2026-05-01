CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall
TARGET = dash
VERSION ?= $(shell cat VERSION)
PKG_NAME = noether-$(VERSION)
SRCS = dash.cpp lexer.cpp parser.cpp analyser.cpp reconciler.cpp interpreter.cpp simulation_engine.cpp
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

package: all
	mkdir -p dist
	tar -czf dist/$(PKG_NAME).tar.gz \
		README.md LICENSE Makefile VERSION \
		$(TARGET) \
		lexer.hpp parser.hpp analyser.hpp reconciler.hpp interpreter.hpp simulation_engine.hpp frame_state.hpp ast.hpp token.hpp \

		lexer.cpp parser.cpp analyser.cpp reconciler.cpp interpreter.cpp simulation_engine.cpp dash.cpp \
		example_v1.noe example_v2.noe
	@echo "Created dist/$(PKG_NAME).tar.gz"
