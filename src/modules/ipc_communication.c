// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/ipc_communication.h"
#include "../include/config.h"
#include "../include/memory.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

// Variables globales de comunicación
int player_pipes[9][2];

// Función para crear pipes de comunicación
int create_player_pipes() {
  for (int i = 0; i < (int)game_state->cantPlayers; i++) {
    if (pipe(player_pipes[i]) == -1) {
      perror("pipe");
      return -1;
    }
    // Poner el extremo de lectura en no bloqueante para evitar detener el loop
    int flags = fcntl(player_pipes[i][0], F_GETFL, 0);
    if (flags != -1)
      fcntl(player_pipes[i][0], F_SETFL, flags | O_NONBLOCK);
  }
  return 0;
}

// Función para crear proceso de vista
pid_t create_view_process(int width, int height) {
  pid_t view_pid = fork();

  if (view_pid == -1) {
    perror("fork view");
    return -1;
  }

  if (view_pid == 0) {
    // Proceso hijo - ejecutar vista
    char width_str[16], height_str[16];
    snprintf(width_str, sizeof(width_str), "%d", width);
    snprintf(height_str, sizeof(height_str), "%d", height);

    char *args[] = {"view", width_str, height_str, NULL};
    execve(view_path, args, environ);
    perror("execve view");
    exit(EXIT_FAILURE);
  }

  return view_pid;
}

// Función para crear procesos de jugadores
pid_t create_player_process(const char *player_executable, int pipe_fd) {
  pid_t player_pid = fork();

  if (player_pid == -1) {
    perror("fork player");
    return -1;
  }

  if (player_pid == 0) {
    // Proceso hijo
    // Redirigir stdout del jugador al extremo de escritura del pipe
    if (dup2(pipe_fd, STDOUT_FILENO) == -1) {
      perror("dup2 player stdout");
      exit(EXIT_FAILURE);
    }
    // Cerrar descriptor de escritura (ya duplicado)
    close(pipe_fd);

    // Pasar ancho y alto como argumentos (compatible con player_cente)
    char width_str[16], height_str[16];
    snprintf(width_str, sizeof(width_str), "%d", (int)game_state->width);
    snprintf(height_str, sizeof(height_str), "%d", (int)game_state->height);

    char *args[] = {(char *)player_executable, width_str, height_str, NULL};
    execve(player_executable, args, environ);
    perror("execve player");
    exit(EXIT_FAILURE);
  }

  return player_pid;
}
