#ifndef GAME_H
#define GAME_H

#include "player.h"

#define SHM_STATE "/game_state"
#define SHM_SEM "/game_sync"

typedef struct {
  unsigned short width;     // Ancho del tablero
  unsigned short high;      // Alto del tablero
  unsigned int cantPlayers; // Cantidad de jugadores
  player players[9];        // Lista de jugadores
  char ended;               // Indica si el juego se ha terminado
  int startBoard[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} game;

#endif // GAME_H
