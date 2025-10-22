# ===== Minimal Makefile + Debug + Color + Dependency Tracking =====

CXX       = g++
MODE     ?= release
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

# Compiler flags based on mode
ifeq ($(MODE),debug)
  CXXFLAGS = -std=c++17 -O0 -g3 -Wall -Wextra -fPIC -MMD -MP -Iinclude
else
  CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -fPIC -MMD -MP -Iinclude
endif

LDFLAGS   = -shared

SRC_DIR   = src
OBJ_DIR   = build/$(MODE)

SOURCES = \
  $(SRC_DIR)/Random.cpp \
  $(SRC_DIR)/RV32Decoder.cpp \
  $(SRC_DIR)/RV32Encoder.cpp \
  $(SRC_DIR)/CompressedMutator.cpp \
  $(SRC_DIR)/RV32Mutator.cpp \
  $(SRC_DIR)/AFLInterface.cpp \
  $(SRC_DIR)/LegalCheck.cpp

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    = $(OBJECTS:.o=.d)

TARGET = librv32_mutator.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(OBJ_DIR)
	@echo "$(C_GRN)[LD]$(C_RESET) $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "$(C_BLU)[CXX]$(C_RESET) $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@echo "$(C_YEL)[CLEAN]$(C_RESET) build $(TARGET)"
	@rm -rf build $(TARGET)

.PHONY: all clean
# ======================================================================