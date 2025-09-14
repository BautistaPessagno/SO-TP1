// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/config.h"
#include "include/memory.h"
#include "include/game_init.h"
#include "include/ipc_communication.h"
#include "include/game_loop.h"
#include "include/game_results.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Limpiar cualquier segmento de memoria compartida previo
  shm_unlink(SHM_STATE);
  shm_unlink(SHM_SEM);

  // Variables locales
  int width, height, num_players;
  char *player_executables[9];
  pid_t player_pids[9];
  pid_t view_pid = -1;

  // Parsear y validar argumentos
  if (parse_arguments(argc, argv, &width, &height, &num_players, player_executables) != 0) {
    return EXIT_FAILURE;
  }
  validate_parameters(width, height, num_players);

  printf("Iniciando ChompChamps - %dx%d tablero, %d jugadores\n", width, height, num_players);

  // Crear memoria compartida
  if (create_game_shared_memory(width, height, num_players) == -1) {
    return EXIT_FAILURE;
  }

  if (create_semaphore_shared_memory() == -1) {
    return EXIT_FAILURE;
  }

  // Inicializar jugadores y tablero
  initialize_players(player_executables, num_players);
  initialize_board();

  // Crear pipes de comunicación
  if (create_player_pipes() == -1) {
    return EXIT_FAILURE;
  }

  // Crear proceso de la vista (solo si se especificó)
  if (view_path != NULL) {
    view_pid = create_view_process(width, height);
    if (view_pid == -1) {
      return EXIT_FAILURE;
    }
  }

  // Crear procesos de jugadores
  for (int i = 0; i < num_players; i++) {
    player_pids[i] = create_player_process(player_executables[i], player_pipes[i][1]);

    if (player_pids[i] == -1) {
      return EXIT_FAILURE;
    }
    // Registrar PID del jugador en el estado compartido (protegido por D)
    sem_wait(&game_semaphores->game_state_mutex);
    game_state->players[i].pid = player_pids[i];
    sem_post(&game_semaphores->game_state_mutex);
    // Cerrar el extremo de escritura del pipe en el padre
    close(player_pipes[i][1]);
  }

  printf("Todos los procesos creados. Iniciando juego...\n");

  // Señalar a la vista que puede empezar (solo si hay vista)
  if (view_pid != -1) {
    sem_post(&game_semaphores->game_view_updated);
  }

  // Ejecutar el loop principal del juego
  run_game_loop(num_players, view_pid);

  // Calcular y mostrar resultados
  int winner = calculate_winner(num_players);
  wait_for_processes(num_players, player_pids, view_pid);
  print_final_results(num_players, winner);

  // Limpiar recursos
  cleanup_memory(width, height);

  printf("Master terminado.\n");
  return EXIT_SUCCESS;
}

