#ifndef PLAYER_H
#define PLAYER_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>

typedef struct {
    char playerName[16];      // Nombre del jugador
    unsigned int score;       // Puntaje
    unsigned int invalidMove; // Cantidad de solicitudes de movimientos inválidas
                              // realizadas
    unsigned int validMove; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short qx, qy; // Coordenadas x e y en el tablero
    pid_t pid;             // Identificador de proceso
    char blocked;          // Indica si el jugador está bloqueado
    int lastMove;          // Último movimiento (0..7) o -1 si ninguno //ESTE LO AGREGUE YO... ELIMINAR DESPUES
  } player;

#endif // PLAYER_H
  