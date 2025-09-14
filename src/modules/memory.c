// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/memory.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Variables globales de memoria
int game_shm_fd, sem_shm_fd;
game *game_state;
semaphore_struct *game_semaphores;

// Función para crear memoria compartida del juego
int create_game_shared_memory(int width, int height, int num_players) {
  size_t game_size = sizeof(game) + (width * height * sizeof(int));

  // Crear memoria compartida para el estado del juego
  game_shm_fd = shm_open(SHM_STATE, O_CREAT | O_RDWR, 0666);
  if (game_shm_fd == -1) {
    perror("shm_open game_state");
    return -1;
  }

  // Establecer el tamaño
  if (ftruncate(game_shm_fd, game_size) == -1) {
    perror("ftruncate game_state");
    return -1;
  }

  // Mapear memoria
  game_state =
      mmap(NULL, game_size, PROT_READ | PROT_WRITE, MAP_SHARED, game_shm_fd, 0);
  if (game_state == MAP_FAILED) {
    perror("mmap game_state");
    return -1;
  }

  // Inicializar estado del juego
  game_state->width = width;
  game_state->height = height;
  game_state->cantPlayers = num_players;
  game_state->ended = 0;

  return 0;
}

// Función para crear memoria compartida de semáforos
int create_semaphore_shared_memory() {
  // Crear memoria compartida para la estructura de semáforos
  sem_shm_fd = shm_open(SHM_SEM, O_CREAT | O_RDWR, 0666);
  if (sem_shm_fd == -1) {
    perror("shm_open semaphores");
    return -1;
  }

  if (ftruncate(sem_shm_fd, (off_t)sizeof(semaphore_struct)) == -1) {
    perror("ftruncate semaphores");
    return -1;
  }

  game_semaphores = mmap(NULL, sizeof(semaphore_struct), PROT_READ | PROT_WRITE,
                         MAP_SHARED, sem_shm_fd, 0);
  if (game_semaphores == MAP_FAILED) {
    perror("mmap semaphores");
    return -1;
  }

  // Inicializar semáforos como compartidos entre procesos (pshared=1)
  if (sem_init(&game_semaphores->A, 1, 0) == -1) {
    perror("sem_init A");
    return -1;
  }
  if (sem_init(&game_semaphores->B, 1, 0) == -1) {
    perror("sem_init B");
    return -1;
  }
  if (sem_init(&game_semaphores->C, 1, 1) == -1) {
    perror("sem_init C");
    return -1;
  }
  if (sem_init(&game_semaphores->D, 1, 1) == -1) {
    perror("sem_init D");
    return -1;
  }
  if (sem_init(&game_semaphores->E, 1, 1) == -1) {
    perror("sem_init E");
    return -1;
  }
  game_semaphores->F = 0;
  for (int i = 0; i < 9; i++) {
    if (sem_init(&game_semaphores->G[i], 1, 0) == -1) {
      perror("sem_init G");
      return -1;
    }
  }
  return 0;
}

void cleanup_memory(int width, int height) {
  // Limpiar recursos (después de imprimir resultados)
  munmap(game_state, sizeof(game) + (width * height * sizeof(int)));
  // Destruir semáforos antes de liberar la memoria compartida
  sem_destroy(&game_semaphores->A);
  sem_destroy(&game_semaphores->B);
  sem_destroy(&game_semaphores->C);
  sem_destroy(&game_semaphores->D);
  sem_destroy(&game_semaphores->E);
  for (int i = 0; i < 9; i++) {
    sem_destroy(&game_semaphores->G[i]);
  }
  munmap(game_semaphores, sizeof(semaphore_struct));
  close(game_shm_fd);
  close(sem_shm_fd);
  shm_unlink(SHM_STATE);
  shm_unlink(SHM_SEM);
}
