SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

TARGET    := falling_sand
EXE       := $(BIN_DIR)/$(TARGET)

ifeq ($(MODE),release)
    BUILD_MODE := release
    CFLAGS     := -O3 -DNDEBUG
else
    BUILD_MODE := debug
    CFLAGS     := -g -O0 -DDEBUG
endif

CFLAGS       += -Wall -std=c11 -Iinclude
LDFLAGS      := -lm

ifneq ($(shell command -v pkg-config 2>/dev/null),)
    SDL_CFLAGS  := $(shell pkg-config --cflags sdl3 2>/dev/null)
    SDL_LDFLAGS := $(shell pkg-config --libs sdl3 2>/dev/null)
else ifneq ($(shell command -v sdl3-config 2>/dev/null),)
    SDL_CFLAGS  := $(shell sdl3-config --cflags)
    SDL_LDFLAGS := $(shell sdl3-config --libs)
endif

CFLAGS  += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LDFLAGS)

SRCS := $(shell find $(SRC_DIR) -name '*.c' | sort)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all debug release run clean distclean info

all: $(EXE)

debug release:
	@$(MAKE) MODE=$@ all

$(EXE): $(OBJS) | $(BIN_DIR)
	@echo "  LD $@"
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

run: $(EXE)
	./$(EXE)

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEPS)
