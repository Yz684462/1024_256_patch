CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
INCLUDES = -Iinclude
SRCDIR = src
INCDIR = include

# Source files
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/patch.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/handle.cpp $(SRCDIR)/vector_context.cpp $(SRCDIR)/cpu.cpp $(SRCDIR)/config.cpp $(SRCDIR)/globals.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = framework

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) -ldl -lpthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install:
	@echo "Installing framework..."
	cp $(TARGET) /usr/local/bin/

help:
	@echo "Available targets:"
	@echo "  all     - Build the framework"
	@echo "  clean   - Remove built files"
	@echo "  install - Install to /usr/local/bin"
	@echo "  help    - Show this help message"
