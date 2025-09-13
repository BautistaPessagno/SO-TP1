#ifndef GAME_RESULTS_H
#define GAME_RESULTS_H

#include "game.h"

// Funciones de resultados y finalización
int calculate_winner(int num_players);
void print_final_results(int num_players, int winner);
void wait_for_processes(int num_players, pid_t player_pids[], pid_t view_pid);

#endif // GAME_RESULTS_H
