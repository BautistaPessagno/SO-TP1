// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "include/cente_board.h"
#include <stdlib.h>
#include <string.h>

static inline int idx(const Board *b, int x, int y) { return y * b->width + x; }

Board *board_create_from_shared(int width, int height, int num_players,
                                const unsigned short *head_x,
                                const unsigned short *head_y,
                                const unsigned char *blocked,
                                const unsigned int *score, const int *cells,
                                int self_player) {
  Board *b = (Board *)calloc(1, sizeof(Board));
  if (!b)
    return NULL;
  b->width = width;
  b->height = height;
  b->num_players = num_players;
  b->current_player = self_player;
  for (int i = 0; i < num_players; i++) {
    b->head_x[i] = head_x[i];
    b->head_y[i] = head_y[i];
    b->blocked[i] = blocked[i];
    b->score[i] = score[i];
  }
  size_t n = (size_t)width * (size_t)height;
  b->cells = (int *)malloc(n * sizeof(int));
  if (!b->cells) {
    free(b);
    return NULL;
  }
  memcpy(b->cells, cells, n * sizeof(int));
  return b;
}

void board_destroy(Board *b) {
  if (!b)
    return;
  free(b->cells);
  free(b);
}

bool board_is_inside(const Board *b, int x, int y) {
  return x >= 0 && x < b->width && y >= 0 && y < b->height;
}

static bool occupied_or_blocked(const Board *b, int x, int y, int self_id) {
  int cell = b->cells[idx(b, x, y)];
  if (cell <= 0)
    return true;
  for (int j = 0; j < b->num_players; j++) {
    if (j == self_id)
      continue;
    if (!b->blocked[j] && b->head_x[j] == (unsigned short)x &&
        b->head_y[j] == (unsigned short)y) {
      return true;
    }
  }
  return false;
}

bool board_is_legal(const Board *b, int player_id, CenteMove m) {
  if (!board_is_inside(b, m.x, m.y))
    return false;
  return !occupied_or_blocked(b, m.x, m.y, player_id);
}

int board_legal_moves(const Board *b, int player_id, CenteMove out[],
                      int max_out) {
  static const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
  int x = b->head_x[player_id];
  int y = b->head_y[player_id];
  int k = 0;
  for (int d = 0; d < 8; d++) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (board_is_inside(b, nx, ny) &&
        !occupied_or_blocked(b, nx, ny, player_id)) {
      if (k < max_out) {
        out[k].x = nx;
        out[k].y = ny;
      }
      k++;
    }
  }
  return k;
}

void board_apply_move(Board *b, int player_id, CenteMove m) {
  int x = b->head_x[player_id];
  int y = b->head_y[player_id];
  int dest = idx(b, m.x, m.y);
  int val = b->cells[dest];
  if (val > 0)
    b->score[player_id] += (unsigned int)val;
  b->cells[idx(b, x, y)] = -player_id;
  b->head_x[player_id] = (unsigned short)m.x;
  b->head_y[player_id] = (unsigned short)m.y;
  b->cells[dest] = -player_id;
}

// Simple Zobrist-like rolling hash based on cell occupancy and heads
uint64_t board_hash(const Board *b) {
  uint64_t h = 1469598103934665603ULL; // FNV offset
  uint64_t prime = 1099511628211ULL;
  size_t n = (size_t)b->width * (size_t)b->height;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint64_t)(b->cells[i] * 1315423911u);
    h *= prime;
  }
  for (int p = 0; p < b->num_players; p++) {
    h ^= (uint64_t)((b->head_x[p] << 16) ^ b->head_y[p] ^ (p << 24));
    h *= prime;
  }
  h ^= (uint64_t)b->current_player * 0x9e3779b97f4a7c15ULL;
  return h;
}
