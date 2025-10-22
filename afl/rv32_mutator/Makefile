
# Makefile — builds librv32_mutator.so (Hybrid RV32 mutator with RV64 stub)
CXX       ?= g++
CXXFLAGS  ?= -O2 -fPIC -std=gnu++17 -Wall -Wextra -Wno-unused-parameter
LDFLAGS   ?= -shared
TARGET     = librv32_mutator.so

INCLUDES   = -Iinclude
SRC_DIR    = src
OBJ_DIR    = build

SOURCES    = $(SRC_DIR)/Random.cpp \
             $(SRC_DIR)/Instruction.cpp \
             $(SRC_DIR)/RV32Decoder.cpp \
             $(SRC_DIR)/RV32Encoder.cpp \
             $(SRC_DIR)/CompressedMutator.cpp \
             $(SRC_DIR)/RV32Mutator.cpp \
             $(SRC_DIR)/AFLInterface.cpp \
             $(SRC_DIR)/LegalCheck.cpp \
             $(SRC_DIR)/FormatBridge.cpp
             
OBJECTS    = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
