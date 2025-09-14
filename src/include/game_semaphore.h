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

typedef struct {
    sem_t game_view_updated; // Master -> View: hay cambios
    sem_t game_view_finished; // View -> Master: terminó de imprimir
    sem_t game_master_mutex; // Mutex para evitar inanición del máster al acceder al estado
    sem_t game_state_mutex; // Mutex del estado del juego
    sem_t game_reader_mutex; // Mutex para la siguiente variable
    unsigned int game_players_count; // cantidad de jugadores leyendo el estado del juego
    sem_t game_players_sem[9]; // Semáforos por jugador
} semaphore_struct;

#endif
