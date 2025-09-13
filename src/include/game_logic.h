#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game.h"

// Constantes de direcciones
extern const int dx[8];
extern const int dy[8];

// Funciones de lógica del juego
int is_inside(int x, int y);
int is_occupied(int x, int y, int self_id);
int has_valid_move(int pid);
void apply_player_move(int pid, int direction);
unsigned long long current_millis();

#endif // GAME_LOGIC_H
