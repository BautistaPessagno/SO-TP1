// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/game_init.h"
#include "../include/config.h"
#include "../include/memory.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Función para inicializar el tablero con valores aleatorios
void initialize_board() {
  srand(seed);

  // Llenar tablero con valores aleatorios (1-9)
  for (int i = 0; i < game_state->width * game_state->height; i++) {
    game_state->startBoard[i] = (rand() % 9) + 1;
  }

  // Limpiar posiciones de jugadores (valor 0)
  for (int i = 0; i < (int)game_state->cantPlayers; i++) {
    int pos = game_state->players[i].qy * game_state->width +
              game_state->players[i].qx;
    game_state->startBoard[pos] = -i;
  }
}

void initialize_players(char *player_executables[], int num_players) {
  (void)num_players; // num_players coincide con game_state->cantPlayers

  for (int i = 0; i < (int)game_state->cantPlayers; i++) {
    // Derivar el nombre a partir del ejecutable asignado (basename)
    const char *path = player_executables[i];
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    if (strncmp(base, "./", 2) == 0)
      base += 2;
    // Copiar truncando al tamaño disponible
    snprintf(game_state->players[i].playerName,
             sizeof(game_state->players[i].playerName), "%s", base);
    game_state->players[i].score = 0;
    game_state->players[i].invalidMove = 0;
    game_state->players[i].validMove = 0;
    game_state->players[i].blocked = 0;

    // elegir una celda libre aleatoria que no se superponga con otros players
    bool posicion_valida;
    do {
      posicion_valida = true;
      game_state->players[i].qx = rand() % game_state->width;
      game_state->players[i].qy = rand() % game_state->height;

      for (int j = 0; j < i; j++) {
        if (game_state->players[i].qx == game_state->players[j].qx &&
            game_state->players[i].qy == game_state->players[j].qy) {
          posicion_valida = false;
          break;
        }
      }
    } while (!posicion_valida);
  }
}
