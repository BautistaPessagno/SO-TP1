

#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

int calculate_value(game *game_state, int player_id, int column, int row, int step){
    // Base case: if no more steps, return 0
    if (step <= 0) {
        return 0;
    }
    
    int width = game_state->width;
    int height = game_state->height;
    int total_value = 0;
    
    // Check all 8 adjacent directions
    for (int dir = 0; dir < 8; dir++) {
        int new_x = column + dx[dir];
        int new_y = row + dy[dir];
        
        // Check if out of bounds (border cell)
        if (new_x < 0 || new_x >= width || new_y < 0 || new_y >= height) {
            total_value -= 1; // Subtract 1 for border cells
            continue;
        }
        
        // Check if cell is occupied by another player
        int occupied = 0;
        for (int i = 0; i < (int)game_state->cantPlayers; i++) {
            if (i != player_id && 
                game_state->players[i].qx == new_x && 
                game_state->players[i].qy == new_y &&
                !game_state->players[i].blocked) {
                occupied = 1;
                break;
            }
        }
        
        // If occupied, don't sum (skip this cell)
        if (occupied) {
            continue;
        }
        
        // Get the cell value from the board
        int cell_value = game_state->startBoard[new_y * width + new_x];
        
        // If it's a body (negative value), don't sum
        if (cell_value < 0) {
            continue;
        }
        
        // Sum the value of the non-occupied cell (multiply by 10 for immediate reward emphasis)
        total_value += cell_value * 25;
        
        // Recursively calculate value for this cell with reduced step
        // Apply a multiplier that favors immediate rewards (greedy behavior)
        double multiplier = 0.7; // Constant multiplier that favors early moves
        int recursive_value = calculate_value(game_state, player_id, new_x, new_y, step - 1);
        total_value += (int)(recursive_value * multiplier);
    }
    
    return total_value;
}

int choose_clever_move(game *game_state, int player_id){
    player *me = &game_state->players[player_id];
    int my_x = me->qx;
    int my_y = me->qy;
    int width = game_state->width;
    int height = game_state->height;
    
    // Array to store values for each possible move
    int move_values[8];
    int valid_moves = 0;
    
    // Check all 8 directions for legal moves
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        
        // Check if move is within bounds
        if (new_x < 0 || new_x >= width || new_y < 0 || new_y >= height) {
            move_values[dir] = -1000000000; // Very large negative value for invalid moves
            continue;
        }
        
        // Check if cell is occupied by another player's head
        int occupied_head = 0;
        for (int i = 0; i < (int)game_state->cantPlayers; i++) {
            if (i != player_id && 
                game_state->players[i].qx == new_x && 
                game_state->players[i].qy == new_y &&
                !game_state->players[i].blocked) {
                occupied_head = 1;
                break;
            }
        }
        if (occupied_head) {
            move_values[dir] = -1000000000; // Very large negative value for occupied moves
            continue;
        }
        
        // Check if cell is a body (negative value) - not allowed
        int cell_value = game_state->startBoard[new_y * width + new_x];
        if (cell_value < 0) {
            move_values[dir] = -1000000000; // Very large negative value for body cells
            continue;
        }
        
        // This is a legal move, calculate its value
        // Use a reasonable number of steps for the recursive calculation
        int steps = 4; // Adjust this value to control recursion depth
        move_values[dir] = calculate_value(game_state, player_id, new_x, new_y, steps);
        valid_moves++;
    }
    
    // If no valid moves found, return -1
    if (valid_moves == 0) {
        return -1;
    }
    
    // Find the direction with the highest value
    int best_direction = -1;
    int best_value = -1000000000; // Initialize with a very low value
    
    for (int dir = 0; dir < 8; dir++) {
        if (move_values[dir] > best_value) {
            best_value = move_values[dir];
            best_direction = dir;
        }
    }
    
    return best_direction;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
      fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
      return EXIT_FAILURE;
    }
  
    (void)argv; // width/height passed for compatibility; read from shared memory
  
    // Open shared memory and semaphores
    game *game_state = open_shared_memory();
    semaphore_struct *sem_state = open_semaphore_memory();
  
    if (!game_state || !sem_state) {
      fprintf(stderr,
              "player_greedy: Failed to open shared memory or semaphores\n");
      return EXIT_FAILURE;
    }
  
    srand((unsigned int)(time(NULL) ^ getpid()));
  
    // Determine own player_id by PID (retry briefly)
    int player_id = -1;
    pid_t self = getpid();
    for (int attempt = 0; attempt < 200 && player_id < 0; attempt++) {
      if (acquire_read_access(sem_state) == 0) {
        for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
          if (game_state->players[i].pid == self) {
            player_id = (int)i;
            break;
          }
        }
        release_read_access(sem_state);
      }
      //if (player_id < 0)
        //{ struct timespec ts = {0, 1000000}; nanosleep(&ts, NULL); }
    }
    if (player_id < 0) {
      // Could not find myself in shared state
      close_semaphore_memory(sem_state);
      size_t sz =
          sizeof(game) + (game_state->width * game_state->height * sizeof(int));
      close_shared_memory(game_state, sz);
      return EXIT_FAILURE;
    }
  
    int prev_count = -1;
    while (!game_state->ended) {
      // Leer estado: si estoy bloqueado, cerrar pipe y salir
      if (acquire_read_access(sem_state) == -1) {
        break;
      }
      int am_blocked = game_state->players[player_id].blocked;
      int count = (int)(game_state->players[player_id].validMove +
                        game_state->players[player_id].invalidMove);
      int skip_write = (count == prev_count);
      prev_count = count;
      int move_direction = choose_clever_move(game_state, player_id);
      if (release_read_access(sem_state) == -1) {
        break;
      }
      if (am_blocked) {
        close(STDOUT_FILENO);
        break;
      }
  
      // Esperar turno concedido por el master
      if (wait_for_turn(sem_state, player_id) == -1) {
        break;
      }
  
      // Si el algoritmo no encontró una dirección, enviar cualquier dirección
      if (move_direction == -1) {
        move_direction = rand() % 8;
      }
  
      if (!skip_write) {
        unsigned char b = (unsigned char)move_direction;
        ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
        if (bytes_written != 1) {
          perror("player_greedy write");
          break;
        }
      }
    }
  
    // No imprimir mensajes de salida para no interferir con la vista
  
    // Clean up
    close_semaphore_memory(sem_state);
    size_t game_size =
        sizeof(game) + (game_state->width * game_state->height * sizeof(int));
    close_shared_memory(game_state, game_size);
    // stdout is managed by OS; do not close explicitly here
  
    return EXIT_SUCCESS;
  }
  