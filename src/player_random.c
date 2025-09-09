#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// Choose a random valid move among neighbors
static int choose_random_move(game *game_state, int player_id) {
  player *me = &game_state->players[player_id];
  int my_x = me->qx;
  int my_y = me->qy;
  int width = game_state->width;
  int height = game_state->height;

  int valid_dirs[8];
  int valid_count = 0;

  for (int dir = 0; dir < 8; dir++) {
    int nx = my_x + dx[dir];
    int ny = my_y + dy[dir];

    // Bounds
    if (nx < 0 || nx >= width || ny < 0 || ny >= height)
      continue;

    // Occupied by another player's head?
    int occupied_head = 0;
    for (int i = 0; i < (int)game_state->cantPlayers; i++) {
      if (i == player_id)
        continue;
      if (!game_state->players[i].blocked && game_state->players[i].qx == nx &&
          game_state->players[i].qy == ny) {
        occupied_head = 1;
        break;
      }
    }
    if (occupied_head)
      continue;

    // Board cell must be strictly positive (master forbids <= 0)
    int cell = game_state->startBoard[ny * width + nx];
    if (cell <= 0)
      continue;

    valid_dirs[valid_count++] = dir;
  }

  if (valid_count == 0)
    return -1;

  int pick = rand() % valid_count;
  return valid_dirs[pick];
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Open shared memory and semaphores
  game *game_state = open_shared_memory();
  semaphore_struct *sem_state = open_semaphore_memory();
  if (!game_state || !sem_state) {
    fprintf(stderr, "player_random: Failed to open shared memory or semaphores\n");
    return EXIT_FAILURE;
  }

  srand((unsigned int)(time(NULL) ^ getpid()));

  // Determine own player_id by PID
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
    close_semaphore_memory(sem_state);
    size_t sz = sizeof(game) +
                (game_state->width * game_state->height * sizeof(int));
    close_shared_memory(game_state, sz);
    return EXIT_FAILURE;
  }

  int prev_count = -1;
  while (!game_state->ended && !game_state->players[player_id].blocked) {
    // Wait until master grants this player's turn
    if (wait_for_turn(sem_state, player_id) == -1)
      break;
    if (acquire_read_access(sem_state) == -1)
      break;

    int count = (int)(game_state->players[player_id].validMove +
                      game_state->players[player_id].invalidMove);
    int skip_write = (count == prev_count);
    prev_count = count;

    int move_direction = choose_random_move(game_state, player_id);

    if (release_read_access(sem_state) == -1)
      break;

    if (move_direction == -1) {
      // No valid moves: close stdout to send EOF, master marks blocked
      close(STDOUT_FILENO);
      break;
    }

    if (!skip_write) {
      unsigned char b = (unsigned char)move_direction;
      ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
      if (bytes_written != 1) {
        perror("player_random write");
        break;
      }
    }

    //{ struct timespec ts = {0, 2000000}; nanosleep(&ts, NULL); }
  }

  // Cleanup
  close_semaphore_memory(sem_state);
  size_t game_size =
      sizeof(game) + (game_state->width * game_state->height * sizeof(int));
  close_shared_memory(game_state, game_size);
  return EXIT_SUCCESS;
}
