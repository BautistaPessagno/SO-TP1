
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

// Greedy player algorithm - pick neighbor cell with highest value reachable
static int choose_greedy_move(game *game_state, int player_id) {
  player *me = &game_state->players[player_id];
  int my_x = me->qx;
  int my_y = me->qy;
  int width = game_state->width;
  int height = game_state->high;

  int best_direction = -1;
  int best_value = -1; // board values are >= 0 for free/points, < 0 for bodies

  for (int dir = 0; dir < 8; dir++) {
    int new_x = my_x + dx[dir];
    int new_y = my_y + dy[dir];

    // bounds check
    if (new_x < 0 || new_x >= width || new_y < 0 || new_y >= height) {
      continue;
    }

    // occupied by other player's head?
    int occupied_head = 0;
    for (int i = 0; i < (int)game_state->cantPlayers; i++) {
      if (i != player_id && game_state->players[i].qx == new_x &&
          game_state->players[i].qy == new_y &&
          !game_state->players[i].blocked) {
        occupied_head = 1;
        break;
      }
    }
    if (occupied_head) {
      continue;
    }

    int cell_value = game_state->startBoard[new_y * width + new_x];
    // cannot move onto bodies (negative values). Allow 0 and positive values
    if (cell_value < 0) {
      continue;
    }

    // greedy: maximize the immediate cell value
    if (cell_value > best_value) {
      best_value = cell_value;
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
    if (player_id < 0)
      usleep(1000);
  }
  if (player_id < 0) {
    // Could not find myself in shared state
    close_semaphore_memory(sem_state);
    size_t sz =
        sizeof(game) + (game_state->width * game_state->high * sizeof(int));
    close_shared_memory(game_state, sz);
    return EXIT_FAILURE;
  }

  int prev_count = -1;
  while (!game_state->ended && !game_state->players[player_id].blocked) {

    // Torniquete
    sem_wait(&sem_state->C);
    sem_post(&sem_state->C);
    // Wait until master grants this player's turn
    if (wait_for_turn(sem_state, player_id) == -1) {
      break;
    }
    // Acquire read access to game state
    if (acquire_read_access(sem_state) == -1) {
      break;
    }

    int count = (int)(game_state->players[player_id].validMove +
                      game_state->players[player_id].invalidMove);
    int skip_write = (count == prev_count);
    prev_count = count;

    int move_direction = choose_greedy_move(game_state, player_id);

    // Release read access
    if (release_read_access(sem_state) == -1) {
      break;
    }

    if (!skip_write && move_direction != -1) {
      unsigned char b = (unsigned char)move_direction;
      ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
      if (bytes_written != 1) {
        perror("player_greedy write");
        break;
      }
    }
    // Small sleep to avoid busy spinning
    usleep(2000);
  }

  // No imprimir mensajes de salida para no interferir con la vista

  // Clean up
  close_semaphore_memory(sem_state);
  size_t game_size =
      sizeof(game) + (game_state->width * game_state->high * sizeof(int));
  close_shared_memory(game_state, game_size);
  // stdout is managed by OS; do not close explicitly here

  return EXIT_SUCCESS;
}
