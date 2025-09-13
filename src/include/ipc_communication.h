#ifndef IPC_COMMUNICATION_H
#define IPC_COMMUNICATION_H

#include <sys/types.h>
#include "game.h"

// Variables globales de comunicación
extern int player_pipes[9][2];

// Funciones de comunicación IPC
int create_player_pipes();
pid_t create_view_process(int width, int height);
pid_t create_player_process(const char *player_executable, int pipe_fd);

#endif // IPC_COMMUNICATION_H
