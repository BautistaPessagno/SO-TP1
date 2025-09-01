#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// se va a mover siempre para la derecha
int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <player_id> <pipe_fd>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int width = atoi(argv[1]);
  int hight = atoi(argv[2]);

  game *gameState = open_shared_memory();
  semaphore_struct *semState = open_semaphore_memory();

  if (!gameState) {
    fprintf(stderr, "Player %d: Failed to open shared memory\n", getpid());
    return EXIT_FAILURE;
  }
  if (!semState) {
    fprintf(stderr, "Player %d: Failed to open semaphore\n", getpid());
    return EXIT_FAILURE;
  }

  int playerId = -1;
  pid_t pid = getpid();
  int prev_count = -1;

  // asocio el jugador a su id con el pid
  for (int i = 0; i < gameState->cantPlayers; i++) {
    if (gameState->players[i].pid == pid) {
      playerId = i;
      break;
    }
  }

  if (playerId == -1) {
    fprintf(stderr, "Error, no se encontro el PID de este proceso jugador");
    return EXIT_FAILURE;
  }

  // mientras el juego esta corriendo y este pueda moverse
  while (!gameState->ended && !gameState->players[playerId].blocked) {
    // esperar turno habilitado por el master para este jugador
    if (sem_wait(&semState->G[playerId]) == -1) {
      break;
    }
    // Entrada de lector con molinete (C) para evitar inanición del writer
    // (master)
    sem_wait(&semState->C);
    sem_post(&semState->C);
    // acceder al estado del juego (lectura)
    sem_wait(&semState->E);
    semState->F++;
    if (semState->F == 1) {
      sem_wait(&semState->D);
    }
    sem_post(&semState->E);

    int count = gameState->players[playerId].validMove +
                gameState->players[playerId].invalidMove;
    int skip_write = count == prev_count;
    prev_count = count;

    // para este caso va a ir siempre a la izquierda
    int move = 6;

    sem_wait(&semState->E);
    semState->F--;
    if (semState->F == 0) {
      sem_post(&semState->D);
    }
    sem_post(&semState->E);

    if (!skip_write && move != -1) {
      char buff[1] = {move};
      int written = write(STDOUT_FILENO, buff, 1);
      if (1 != written) {
        perror("child: write");
        exit(EXIT_FAILURE);
      }
    }
  }
  size_t game_size =
      sizeof(game) + (gameState->width * gameState->height * sizeof(int));
  close_shared_memory(gameState, game_size);
  close_semaphore_memory(semState);
}
