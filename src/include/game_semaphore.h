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
    sem_t view_update_signal;     // Master -> View: signals that there are changes to display
    sem_t view_draw_complete;     // View -> Master: signals that the view has finished drawing
    sem_t master_access_mutex;    // Mutex to prevent master starvation when accessing game state
    sem_t game_state_mutex;       // Mutex for the game state (main data protection)
    sem_t reader_count_mutex;     // Mutex for the reader count variable
    unsigned int reader_count;    // Number of readers currently accessing game state
    sem_t player_turn_sem[9];     // Individual semaphores for each player to signal their turn
} semaphore_struct;

#endif
