# Makefile for local compilation of ChompChamps
# This allows compiling directly with gcc without Docker

# Compiler and flags
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -O2 -I./include
LDFLAGS := -pthread

# Detect ncurses flags (prefer pkg-config). Fallback to -lncurses
NCURSES_CFLAGS := $(shell pkg-config --cflags ncurses 2>/dev/null || pkg-config --cflags ncursesw 2>/dev/null)
NCURSES_LIBS := $(shell pkg-config --libs ncurses 2>/dev/null || pkg-config --libs ncursesw 2>/dev/null)

CFLAGS += $(NCURSES_CFLAGS)
ifneq ($(strip $(NCURSES_LIBS)),)
LDFLAGS += $(NCURSES_LIBS)
else
LDFLAGS += -lncurses
endif

# Platform specific fixes for macOS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # On macOS, we might need to adjust some flags
    CFLAGS += -D_DARWIN_C_SOURCE
else
    # On POSIX systems, expose setenv and other POSIX APIs under strict C stds
    CFLAGS += -D_POSIX_C_SOURCE=200112L
endif

# Source files
IPC_SOURCES := ipc/shared_memory.c ipc/semaphore.c ipc/pipes.c
MAIN_SOURCES := master.c view.c player1.c player2.c player_greedy.c player_clever.c player_competitive.c player_random.c testPlayer.c

# Object files
IPC_OBJECTS := $(IPC_SOURCES:.c=.o)
OBJ_DIR := obj
IPC_OBJ := $(addprefix $(OBJ_DIR)/, $(IPC_OBJECTS))

# Executables
EXECUTABLES := master view player1 player2 player_greedy player_clever player_competitive player_random testPlayer

# Default target
all: $(EXECUTABLES)
	@echo "All executables compiled successfully!"
	@echo "Available executables:"
	@ls -la $(EXECUTABLES)

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/ipc

# Compile IPC library objects
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile master
master: master.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile view (needs ncurses)
view: view.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile player1
player1: player1.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile player2
player2: player2.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile player_greedy
player_greedy: player_greedy.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile player_clever
player_clever: player_clever.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile player_competitive
player_competitive: player_competitive.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm

# Compile player_random
player_random: player_random.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile testPlayer
testPlayer: testPlayer.c $(IPC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Individual targets
build-master: master
	@echo "Master compiled successfully!"

build-view: view
	@echo "View compiled successfully!"

build-players: player1 player2 player_greedy player_clever player_competitive player_random testPlayer
	@echo "Players compiled successfully!"

build-player1: player1
	@echo "Player1 compiled successfully!"

build-player2: player2
	@echo "Player2 compiled successfully!"

# Compile IPC libraries only
ipc: $(IPC_OBJ)
	@echo "IPC libraries compiled successfully!"

# Clean build artifacts
clean:
	rm -f $(EXECUTABLES)
	rm -rf $(OBJ_DIR)
	@echo "Clean completed!"

# Test compilation
test: all
	@echo "Testing executables..."
	@for exe in $(EXECUTABLES); do \
		if [ -x "./$$exe" ]; then \
			echo "✓ $$exe is executable"; \
		else \
			echo "✗ $$exe is not executable"; \
			exit 1; \
		fi; \
	done
	@echo "All tests passed!"

# Show build information
info:
	@echo "ChompChamps Local Build System"
	@echo "=============================="
	@echo "Compiler: $(CC)"
	@echo "C Flags: $(CFLAGS)"
	@echo "Linker Flags: $(LDFLAGS)"
	@echo "Platform: $(UNAME_S)"
	@echo "Object Directory: $(OBJ_DIR)"
	@echo "Executables: $(EXECUTABLES)"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build all executables (default)"
	@echo "  master       - Build master executable"
	@echo "  view         - Build view executable"
	@echo "  player1      - Build player1 executable"
	@echo "  player2      - Build player2 executable"
	@echo "  player_greedy - Build greedy player executable"
	@echo "  player_clever - Build clever player executable"
	@echo "  player_competitive - Build competitive player executable"
	@echo "  player_random - Build random player executable"
	@echo "  build-players - Build both players"
	@echo "  ipc          - Build IPC libraries only"
	@echo "  test         - Test that all executables work"
	@echo "  clean        - Remove build artifacts"
	@echo "  info         - Show this information"

# Check dependencies
deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null && echo "✓ GCC/Clang found: $$(which $(CC))" || echo "✗ GCC/Clang not found"
	@pkg-config --exists ncurses && echo "✓ ncurses found" || echo "✗ ncurses not found"
	@which make > /dev/null && echo "✓ make found: $$(which make)" || echo "✗ make not found"
	@echo "Dependencies check complete!"

# Debug target
debug: CFLAGS += -g3 -O0 -DDEBUG
debug: all
	@echo "Debug build completed!"

# Release target  
release: CFLAGS += -O3 -DNDEBUG
release: all
	@echo "Release build completed!"

# Help target
help: info

.PHONY: all clean test info deps debug release help build-master build-view build-players build-player1 build-player2 ipc
