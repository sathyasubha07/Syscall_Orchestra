CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -D_GNU_SOURCE -Iinclude -g -pthread
LDFLAGS = -pthread -lm

# Probe for ALSA library (libasound)
ALSA_LIBS := $(shell pkg-config --libs alsa 2>/dev/null || echo "")
ALSA_CFLAGS := $(shell pkg-config --cflags alsa 2>/dev/null || echo "")

ifneq ($(ALSA_LIBS),)
    CFLAGS += $(ALSA_CFLAGS) -DHAS_ALSA
    LDFLAGS += $(ALSA_LIBS)
endif

# Source and Object files
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
TARGET_SRC_DIR = targets
TARGET_BIN_DIR = targets

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

TARGETS_SRC = $(wildcard $(TARGET_SRC_DIR)/*.c)
TARGETS_BIN = $(patsubst $(TARGET_SRC_DIR)/%.c, $(TARGET_BIN_DIR)/%, $(TARGETS_SRC))

MAIN_BIN = syscall_orchestra

# Default target
all: $(MAIN_BIN) targets

$(MAIN_BIN): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Build target programs
targets: $(TARGETS_BIN)

$(TARGET_BIN_DIR)/%: $(TARGET_SRC_DIR)/%.c
	$(CC) -Wall -Wextra -Wpedantic -std=c99 -D_GNU_SOURCE -g $< -o $@

# Run tests
test: all targets
	@chmod +x scripts/*.sh
	./scripts/run_tests.sh

# Debug build configuration
debug: CFLAGS += -DDEBUG -O0
debug: all

# Clean up build artifacts
clean:
	rm -rf $(OBJ_DIR) $(MAIN_BIN) $(TARGETS_BIN) *.tmp mixed_test.tmp test_syscall_orchestra.tmp
	@echo "Cleanup completed."

.PHONY: all clean targets test debug
