// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/game_results.h"
#include "../include/memory.h"
#include <stdio.h>
#include <sys/wait.h>

int calculate_winner(int num_players) {
  // Calcular ganador: mayor puntaje; en empate, menor validMove
  int winner = -1;
  unsigned int best_score = 0;
  unsigned int best_valid = 0xFFFFFFFFu;
  for (int i = 0; i < num_players; i++) {
    unsigned int s = game_state->players[i].score;
    unsigned int v = game_state->players[i].validMove;
    if (winner == -1 || s > best_score || (s == best_score && v < best_valid)) {
      winner = i;
      best_score = s;
      best_valid = v;
    }
  }
  return winner;
}

void print_final_results(int num_players, int winner) {
  printf("Resultados finales:\n");
  for (int i = 0; i < num_players; i++) {
    printf(
        "Jugador %c | Puntaje: %u | Válidos: %u | Inválidos: %u | Estado: %s\n",
        'A' + i, game_state->players[i].score, game_state->players[i].validMove,
        game_state->players[i].invalidMove,
        game_state->players[i].blocked ? "BLOQ" : "ACTIVO");
  }
  if (winner >= 0) {
    printf("Ganador: Jugador %c (score=%u, validos=%u)\n", 'A' + winner,
           game_state->players[winner].score,
           game_state->players[winner].validMove);
  } else {
    printf("Sin ganador.\n");
  }
}

void wait_for_processes(int num_players, pid_t player_pids[], pid_t view_pid) {
  // Esperar que terminen todos los procesos
  for (int i = 0; i < num_players; i++) {
    waitpid(player_pids[i], NULL, 0);
  }
  if (view_pid != -1) {
    waitpid(view_pid, NULL, 0);
  }
}
