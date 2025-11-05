# ==========================================================
# ISA-Agnostic Custom Mutator - AFL++ Compatible Makefile
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
CXXFLAGS := -std=c++17 -fPIC -Wall -Wextra -Wno-unused-parameter -Iinclude -I../../include -O2

MUTATOR_CONFIG ?= $(abspath config/mutator.default.yaml)
export MUTATOR_CONFIG

# yaml-cpp detection (pkg-config preferred)
YAML_CFLAGS  ?= $(shell pkg-config --cflags yaml-cpp 2>/dev/null)
YAML_LIBS    ?= $(shell pkg-config --libs yaml-cpp 2>/dev/null)

ifeq ($(strip $(YAML_LIBS)),)
  YAML_LIBS := -lyaml-cpp
endif

CXXFLAGS += $(YAML_CFLAGS)
LDFLAGS  := -shared
LDLIBS   := $(YAML_LIBS)

SRC_DIR   = src
OBJ_DIR   = build/
TARGET    = libisa_mutator.so

SOURCES = \
  $(SRC_DIR)/Random.cpp \
  $(SRC_DIR)/MutatorConfig.cpp \
  $(SRC_DIR)/YamlUtils.cpp \
  $(SRC_DIR)/IsaLoader.cpp \
  $(SRC_DIR)/AFLInterface.cpp \
  $(SRC_DIR)/DebugUtils.cpp \
  $(SRC_DIR)/ISAMutator.cpp \
  $(SRC_DIR)/LegalCheck.cpp \
  $(SRC_DIR)/CompressedMutator.cpp

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "$(C_GRN)[LD] Building $@$(C_RESET)"
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "$(C_BLU)[CXX] Compiling $<$(C_RESET)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(C_YEL)[CLEAN] Removing build files$(C_RESET)"
	rm -rf build $(TARGET)

.PHONY: all clean
# ==========================================================
