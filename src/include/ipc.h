#ifndef IPC_H
#define IPC_H

#include "game.h"
#include "game_semaphore.h" // Incluir la definición correcta

// --- Shared Memory ---
game* open_shared_memory();
void close_shared_memory(game *game_state, size_t size);

// --- Semaphores ---
semaphore_struct* open_semaphore_memory();
void close_semaphore_memory(semaphore_struct *sem_state);
int acquire_read_access(semaphore_struct *sem_state);
int release_read_access(semaphore_struct *sem_state);
int wait_for_turn(semaphore_struct *sem_state, int player_id);

// --- Pipes ---
// (Prototipos para pipes si fueran necesarios)

#endif // IPC_H
