// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/game_loop.h"
#include "../include/config.h"
#include "../include/game_logic.h"
#include "../include/ipc_communication.h"
#include "../include/memory.h"
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void run_game_loop(int num_players, pid_t view_pid) {
  // Loop principal del juego
  unsigned long long last_valid_move_ms = current_millis();
  const unsigned long long INACTIVITY_TIMEOUT_MS =
      5000; // 5 seconds without valid moves
  // Índice de inicio para política round-robin al atender solicitudes pendientes
  int next_rr_index = 0;
  while (!game_state->ended) {
    // Habilitar un turno para cada jugador activo
    for (int i = 0; i < num_players; i++) {
      if (game_state->players[i].blocked)
        continue;
      if (!has_valid_move(i)) {
        // Si no tiene movimientos válidos, bloquear al jugador
        game_state->players[i].blocked = 1;
        continue;
      }
      sem_post(&game_semaphores->game_players_sem[i]);
    }
    // Leer sin bloquear del pipe de cada jugador

    struct pollfd pfds[9];
    int nfds = 0;
    for (int i = 0; i < num_players; i++) {
      pfds[i].fd = player_pipes[i][0];
      pfds[i].events = POLLIN;
      pfds[i].revents = 0;
    }
    nfds = num_players;
    int pret = poll(pfds, nfds, 50); // esperar hasta 50ms por datos
    if (pret > 0) {
      int last_processed = -1;
      // Atender en orden round-robin comenzando desde next_rr_index
      for (int k = 0; k < num_players; k++) {
        int i = (next_rr_index + k) % num_players;
        if (game_state->players[i].blocked)
          continue;
        if (pfds[i].revents & POLLIN) {
          unsigned char move_byte;
          ssize_t r = read(player_pipes[i][0], &move_byte, 1);
          if (r >= 1) {
            int direction = (int)move_byte;
            // Exclusión mutua de escritura del estado del juego (RW-lock)
            sem_wait(&game_semaphores->game_master_mutex);
            sem_wait(&game_semaphores->game_state_mutex);
            unsigned int prev_valid = game_state->players[i].validMove;
            apply_player_move(i, direction);
            if (game_state->players[i].validMove != prev_valid) {
              last_valid_move_ms = current_millis();
            }
            sem_post(&game_semaphores->game_state_mutex);
            sem_post(&game_semaphores->game_master_mutex);
            last_processed = i;
          } else if (r == 0) {
            // EOF: jugador sin más movimientos -> bloquear
            game_state->players[i].blocked = 1;
          } else {
            // Error de lectura. Si es no bloqueante sin datos, ignorar; de lo
            // contrario, bloquear.
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              // sin datos ahora
            } else {
              game_state->players[i].blocked = 1;
            }
          }
        }
        if (pfds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
          // Pipe cerrado o error: considerar bloqueado
          game_state->players[i].blocked = 1;
        }
      }
      if (last_processed != -1) {
        next_rr_index = (last_processed + 1) % num_players;
      }
    }

    // Señalar a la vista que actualice (solo si hay vista)
    if (view_pid != -1) {
      sem_post(&game_semaphores->game_view_updated);

      // Esperar que la vista termine de actualizar
      sem_wait(&game_semaphores->game_view_finished);
    }

    // Pequeño respiro para no saturar CPU y dar tiempo a jugadores
    {
      struct timespec ts = {0, 500000000};
      nanosleep(&ts, NULL);
    } // 500ms

    // Timeout por inactividad de movimientos válidos
    if (!game_state->ended) {
      unsigned long long now_ms = current_millis();
      if (now_ms - last_valid_move_ms >= INACTIVITY_TIMEOUT_MS) {
        game_state->ended = 1;
        if (view_pid != -1) {
          sem_post(&game_semaphores->game_view_updated); // despertar vista
        }
        for (int i = 0; i < num_players; i++) {
          sem_post(&game_semaphores->game_players_sem[i]); // despertar jugadores
        }
      }
    }

    // Verificar condición de fin: todos los jugadores bloqueados
    int all_blocked = 1;
    for (int i = 0; i < num_players; i++) {
      if (!game_state->players[i].blocked) {
        all_blocked = 0;
        break;
      }
    }
    if (all_blocked) {
      game_state->ended = 1;
      // Despertar la vista para que pueda leer ended y salir (solo si hay
      // vista)
      if (view_pid != -1) {
        sem_post(&game_semaphores->game_view_updated);
      }
      // Desbloquear a todos los jugadores por si están esperando turno
      for (int i = 0; i < num_players; i++) {
        sem_post(&game_semaphores->game_players_sem[i]);
      }
    }
  }
}
