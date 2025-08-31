#ifndef GAME_H
#define GAME_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define SHM_STATE "/game_state"
#define SHM_SEM "/game_sync"

typedef struct {
    char playerName[16];      // Nombre del jugador
    unsigned int score;       // Puntaje
    unsigned int invalidMove; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int validMove; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short qx, qy; // Coordenadas x e y en el tablero
    pid_t pid;             // Identificador de proceso
    char blocked;          // Indica si el jugador está bloqueado
} player;

typedef struct {
  unsigned short width;     // Ancho del tablero
  unsigned short height;      // Alto del tablero
  unsigned int cantPlayers; // Cantidad de jugadores
  player players[9];        // Lista de jugadores
  char ended;               // Indica si el juego se ha terminado
  int startBoard[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} game;

#endif // GAME_H
