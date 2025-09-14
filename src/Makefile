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

# Valgrind configuration
VALGRIND := valgrind
VG_COMMON_OPTS := --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=all --num-callers=25 --error-exitcode=42
VG_LOG_DIR := logs/valgrind
VG_LOG_ALL := $(VG_LOG_DIR)/full_run.log
VG_LOG_NO_CHILDREN := $(VG_LOG_DIR)/no_children.log
VG_LOG_VIEW_ONLY := $(VG_LOG_DIR)/view_only.log
VG_LOG_PLAYERS_ONLY := $(VG_LOG_DIR)/players_only.log

# Source files
IPC_SOURCES := ipc/shared_memory.c ipc/semaphore.c ipc/pipes.c
MODULE_SOURCES := modules/config.c modules/memory.c modules/game_init.c modules/ipc_communication.c modules/game_logic.c modules/game_loop.c modules/game_results.c modules/utils.c
MAIN_SOURCES := master.c view.c player1.c player2.c player_greedy.c player_random.c testPlayer.c

# Object files
IPC_OBJECTS := $(IPC_SOURCES:.c=.o)
MODULE_OBJECTS := $(MODULE_SOURCES:.c=.o)
OBJ_DIR := obj
IPC_OBJ := $(addprefix $(OBJ_DIR)/, $(IPC_OBJECTS))
MODULE_OBJ := $(addprefix $(OBJ_DIR)/, $(MODULE_OBJECTS))

# Executables
EXECUTABLES := master view player1 player2 player_greedy player_random testPlayer

# Default target
all: $(EXECUTABLES)
	@echo "All executables compiled successfully!"
	@echo "Available executables:"
	@ls -la $(EXECUTABLES)

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/ipc
	mkdir -p $(OBJ_DIR)/modules

# Compile IPC library objects
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile master
master: master.c $(IPC_OBJ) $(MODULE_OBJ)
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

build-players: player1 player2 player_greedy player_random testPlayer
	@echo "Players compiled successfully!"

build-player1: player1
	@echo "Player1 compiled successfully!"

build-player2: player2
	@echo "Player2 compiled successfully!"

# Compile IPC libraries only
ipc: $(IPC_OBJ)
	@echo "IPC libraries compiled successfully!"

# Compile modules only
modules: $(MODULE_OBJ)
	@echo "Modules compiled successfully!"

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

# Ensure logs directory exists
$(VG_LOG_DIR):
	mkdir -p $(VG_LOG_DIR)

# Valgrind: Full run (master with view and players; children traced)
valgrind-all: all | $(VG_LOG_DIR)
	@echo "Running Valgrind: full run (with children)"
	$(VALGRIND) $(VG_COMMON_OPTS) --trace-children=yes --log-file=$(VG_LOG_ALL) ./master -w 10 -h 10 -d 10 -t 5 -s 1 -v ./view -p ./player1 ./player2
	@echo "Logs saved to $(VG_LOG_ALL)"

# Valgrind: Do not trace children (only master process)
valgrind-no-children: all | $(VG_LOG_DIR)
	@echo "Running Valgrind: master only (no children)"
	$(VALGRIND) $(VG_COMMON_OPTS) --trace-children=no --log-file=$(VG_LOG_NO_CHILDREN) ./master -w 10 -h 10 -d 10 -t 5 -s 1 -v ./view -p ./player1 ./player2
	@echo "Logs saved to $(VG_LOG_NO_CHILDREN)"

# Valgrind: Only include view (players omitted)
valgrind-view-only: all | $(VG_LOG_DIR)
	@echo "Running Valgrind: view only"
	$(VALGRIND) $(VG_COMMON_OPTS) --trace-children=yes --log-file=$(VG_LOG_VIEW_ONLY) ./master -w 10 -h 10 -d 10 -t 5 -s 1 -v ./view -p ./testPlayer
	@echo "Logs saved to $(VG_LOG_VIEW_ONLY)"

# Valgrind: Only include players (no view)
valgrind-players-only: all | $(VG_LOG_DIR)
	@echo "Running Valgrind: players only (no view)"
	$(VALGRIND) $(VG_COMMON_OPTS) --trace-children=yes --log-file=$(VG_LOG_PLAYERS_ONLY) ./master -w 10 -h 10 -d 10 -t 5 -s 1 -p ./player1 ./player2
	@echo "Logs saved to $(VG_LOG_PLAYERS_ONLY)"

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
	@echo "  player_random - Build random player executable"
	@echo "  build-players - Build both players"
	@echo "  ipc          - Build IPC libraries only"
	@echo "  modules      - Build modules only"
	@echo "  test         - Test that all executables work"
	@echo "  valgrind-all           - Run with view and players (trace children)"
	@echo "  valgrind-no-children   - Run master only (no child tracing)"
	@echo "  valgrind-view-only     - Run with view only (testPlayer as player)"
	@echo "  valgrind-players-only  - Run with players only (no view)"
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

.PHONY: all clean test info deps debug release help build-master build-view build-players build-player1 build-player2 ipc modules
