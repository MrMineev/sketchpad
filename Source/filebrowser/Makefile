# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17
INCLUDES = -I /opt/homebrew/Cellar/sfml/2.6.1/include/
LIBS = -L /opt/homebrew/Cellar/sfml/2.6.1/lib/ -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -framework OpenGL

# Output binary name
TARGET = browser 

# Automatically find all .cpp files in the current directory
SRC = $(wildcard $(shell find . -name "*.cpp"))

# Automatically generate .o files for each .cpp file
OBJ = $(SRC:.cpp=.o)

# Build the target (executable)
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

# Compile the .cpp files to .o object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony target for clean
.PHONY: clean


