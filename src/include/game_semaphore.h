#ifndef GAME_SEMAPHORE_H
#define GAME_SEMAPHORE_H

#include <semaphore.h>

// Nombres para semáforos POSIX (compatibles con macOS)
#define SEM_NAME_A "/game_A"
#define SEM_NAME_B "/game_B"
#define SEM_NAME_C "/game_C"
#define SEM_NAME_D "/game_D"
#define SEM_NAME_E "/game_E"
#define SEM_NAME_G_PREFIX "/game_G_" // seguido de índice 0..8

// Estructura que mantiene punteros a semáforos nombrados
typedef struct {
    sem_t *A;        // Master -> View: hay cambios
    sem_t *B;        // View -> Master: terminó de imprimir
    sem_t *C;        // Mutex auxiliar
    sem_t *D;        // Mutex del estado del juego
    sem_t *E;        // Mutex auxiliar
    sem_t *G[9];     // Semáforos por jugador
} semaphore_struct;

#endif // GAME_SEMAPHORE_H
