#include "include/game.h"
#include "include/ipc.h"
#include "include/game_semaphore.h"
#include "include/cente_board.h"
#include "include/cente_mcts.h"
#include "include/cente_influence.h"
#include "include/cente_eval.h"
#include "include/cente_config.h"
#include "include/cente_autotune.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static int choose_cente_move(game *gs, semaphore_struct *sem_state, int player_id) {
    (void)sem_state;
    int w = gs->width, h = gs->height, P = (int)gs->cantPlayers;
    unsigned short hx[CENTE_MAX_PLAYERS];
    unsigned short hy[CENTE_MAX_PLAYERS];
    unsigned char blocked[CENTE_MAX_PLAYERS];
    unsigned int score[CENTE_MAX_PLAYERS];
    for (int i = 0; i < P; i++) {
        hx[i] = gs->players[i].qx; hy[i] = gs->players[i].qy; blocked[i] = (unsigned char)gs->players[i].blocked; score[i] = gs->players[i].score;
    }
    Board *b = board_create_from_shared(w, h, P, hx, hy, blocked, score, gs->startBoard, player_id);
    if (!b) return -1;

    InfluenceMap *inf = influence_create(w, h);
    if (!inf) { board_destroy(b); return -1; }

    cente_size_params base = cente_defaults_for_size(w, h);
    compute_influence_full(b, player_id, base.sigma, inf);
    cente_phase ph; cente_weights wts; phase_detect(b, inf, &ph, &wts);

    cente_size_params tuned;
    float tempo = cente_autotune(b, player_id, inf, ph, &base, &tuned);
    (void)tempo;
    if (tuned.sigma != base.sigma) {
        // Recompute influence with new sigma
        compute_influence_full(b, player_id, tuned.sigma, inf);
    }
    mcts_set_size_params(&tuned);
    CenteMove mv = mcts_select(b, player_id, inf, &wts, &tuned.mcts, tuned.budget_ms);

    // Map move to direction 0..7
    static const int dx[8] = {0,1,1,1,0,-1,-1,-1};
    static const int dy[8] = {-1,-1,0,1,1,1,0,-1};
    int sx = gs->players[player_id].qx; int sy = gs->players[player_id].qy;
    int dir = -1;
    for (int d = 0; d < 8; d++) if (sx + dx[d] == mv.x && sy + dy[d] == mv.y) { dir = d; break; }

    influence_destroy(inf);
    board_destroy(b);
    return dir;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
    return EXIT_FAILURE;
  }
  (void)argv;

  game *game_state = open_shared_memory();
  semaphore_struct *sem_state = open_semaphore_memory();
  if (!game_state || !sem_state) {
    fprintf(stderr, "player_cente: Failed to open shared memory or semaphores\n");
    return EXIT_FAILURE;
  }

  srand((unsigned int)(time(NULL) ^ getpid()));

  int player_id = -1;
  pid_t self = getpid();
  for (int attempt = 0; attempt < 200 && player_id < 0; attempt++) {
    if (acquire_read_access(sem_state) == 0) {
      for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
        if (game_state->players[i].pid == self) { player_id = (int)i; break; }
      }
      release_read_access(sem_state);
    }
  }
  if (player_id < 0) {
    close_semaphore_memory(sem_state);
    size_t sz = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
    close_shared_memory(game_state, sz);
    return EXIT_FAILURE;
  }

  int prev_count = -1;
  while (!game_state->ended) {
    if (wait_for_turn(sem_state, player_id) == -1) { break; }
    if (acquire_read_access(sem_state) == -1) { break; }
    int am_blocked = game_state->players[player_id].blocked;
    int move_direction = choose_cente_move(game_state, sem_state, player_id);
    int count = game_state->players[player_id].validMove + game_state->players[player_id].invalidMove;
    int skip_write = count == prev_count;
    prev_count = count;
    if (release_read_access(sem_state) == -1) { break; }
    if (am_blocked) { close(STDOUT_FILENO); break; }
    if (move_direction == -1) { move_direction = rand() % 8; }
    if (!skip_write) {
        unsigned char b = (unsigned char)move_direction;
        ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
        if (bytes_written != 1) { perror("player_cente write"); break; }
    }
  }

  close_semaphore_memory(sem_state);
  size_t game_size = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
  close_shared_memory(game_state, game_size);
  return EXIT_SUCCESS;
}


