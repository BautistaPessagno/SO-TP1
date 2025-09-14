// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/game_logic.h"
#include "../include/memory.h"
#include <sys/time.h>

// Constantes de direcciones: 0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW
const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

unsigned long long current_millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long long)tv.tv_sec * 1000ULL +
         (unsigned long long)(tv.tv_usec / 1000);
}

int is_inside(int x, int y) {
  return x >= 0 && x < (int)game_state->width && y >= 0 &&
         y < (int)game_state->height;
}

int is_occupied(int x, int y, int self_id) {
  // Ocupado por cuerpo
  int cell = game_state->startBoard[y * game_state->width + x];
  if (cell <= 0)
    return 1;
  // Ocupado por cabeza de algún jugador
  for (int j = 0; j < (int)game_state->cantPlayers; j++) {
    if (j == self_id)
      continue;
    if (game_state->players[j].qx == x && game_state->players[j].qy == y &&
        !game_state->players[j].blocked)
      return 1;
  }
  return 0;
}

int has_valid_move(int pid) {
  int x = game_state->players[pid].qx;
  int y = game_state->players[pid].qy;
  for (int d = 0; d < 8; d++) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (is_inside(nx, ny) && !is_occupied(nx, ny, pid)) {
      return 1;
    }
  }
  return 0;
}

void apply_player_move(int pid, int direction) {
  if (direction < 0 || direction > 7) {
    game_state->players[pid].invalidMove++;
    return;
  }
  int x = game_state->players[pid].qx;
  int y = game_state->players[pid].qy;
  int nx = x + dx[direction];
  int ny = y + dy[direction];
  if (!is_inside(nx, ny) || is_occupied(nx, ny, pid)) {
    game_state->players[pid].invalidMove++;
    // No bloquear de inmediato: permitir que intente otro movimiento en el
    // próximo turno game_state->players[pid].blocked = 1;
    return;
  }
  // Dejar cuerpo en la celda actual
  game_state->startBoard[y * game_state->width + x] = -pid;
  // Puntuar por la celda destino si tiene valor positivo
  int dest_idx = ny * game_state->width + nx;
  int cell_val = game_state->startBoard[dest_idx];
  if (cell_val > 0) {
    game_state->players[pid].score += (unsigned int)cell_val;
  }
  // Mover cabeza
  game_state->players[pid].qx = (unsigned short)nx;
  game_state->players[pid].qy = (unsigned short)ny;
  // lastMove eliminado del estado compartido
  // Limpiar la celda destino para que no muestre puntaje (se verá la cabeza por
  // encima)
  game_state->startBoard[dest_idx] = -pid;
  game_state->players[pid].validMove++;
}
