# ==========================================================
# RV32 Custom Mutator - AFL++ Compatible Makefile
# ==========================================================

# Use AFL compiler for instrumentation compatibility
CXX       ?= afl-clang-fast++
NO_COLOR ?= 0

# Colors (disable with NO_COLOR=1)
ifeq ($(NO_COLOR),1)
  C_RESET :=
  C_BLU   :=
  C_GRN   :=
  C_YEL   :=
else
  C_RESET := \033[0m
  C_BLU   := \033[1;34m
  C_GRN   := \033[1;32m
  C_YEL   := \033[1;33m
endif

# Compiler flags
CXXFLAGS := -std=c++17 -fPIC -Wall -Wextra -Wno-unused-parameter -Iinclude -O2


LDFLAGS  := -shared

SRC_DIR   = src
OBJ_DIR   = build/
TARGET    = librv32_mutator.so

SOURCES = \
  $(SRC_DIR)/Random.cpp \
  $(SRC_DIR)/RV32Decoder.cpp \
  $(SRC_DIR)/RV32Encoder.cpp \
  $(SRC_DIR)/CompressedMutator.cpp \
  $(SRC_DIR)/RV32Mutator.cpp \
  $(SRC_DIR)/AFLInterface.cpp \
  $(SRC_DIR)/LegalCheck.cpp

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "$(C_GRN)[LD] Building $@$(C_RESET)"
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "$(C_BLU)[CXX] Compiling $<$(C_RESET)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(C_YEL)[CLEAN] Removing build files$(C_RESET)"
	rm -rf build $(TARGET)

.PHONY: all clean
# ==========================================================