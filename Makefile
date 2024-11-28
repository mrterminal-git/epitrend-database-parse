# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Iinclude -Ilib/Eigen -Ilib/URT/include -Ilib/Boost -Wall -Wextra -O2 -DUSE_EIGEN
LDFLAGS = 

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib

# Files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
URT_SRCS = $(wildcard $(LIB_DIR)/URT/src/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS)) \
       $(patsubst $(LIB_DIR)/URT/src/%.cpp, $(OBJ_DIR)/URT_%.o, $(URT_SRCS))
TARGET = $(BIN_DIR)/main

# Default target
all: $(TARGET)

# Create target binary
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile object files from your source code
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile object files from URT source code
$(OBJ_DIR)/URT_%.o: $(LIB_DIR)/URT/src/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Run the program
run: all
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean run
