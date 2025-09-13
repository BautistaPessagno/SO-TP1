#ifndef MEMORY_H
#define MEMORY_H

#include "game.h"
#include "game_semaphore.h"

// Variables globales de memoria
extern int game_shm_fd, sem_shm_fd;
extern game *game_state;
extern semaphore_struct *game_semaphores;

// Funciones de gestión de memoria
int create_game_shared_memory(int width, int height, int num_players);
int create_semaphore_shared_memory();
void cleanup_memory(int width, int height);

#endif // MEMORY_H
