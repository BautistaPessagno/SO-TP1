#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "game.h"
#include "game_semaphore.h"

// Función principal del loop del juego
void run_game_loop(int num_players, pid_t view_pid);

#endif // GAME_LOOP_H
