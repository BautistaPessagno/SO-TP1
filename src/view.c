// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"
#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Prototipos de funciones
void init_ncurses();
void init_colors();
void draw_stats(WINDOW *win, game *game_state);
void draw_board(WINDOW *win, game *game_state);
void cleanup_ncurses();
char playersChar[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};

int main(int argc, char *argv[]) {
  (void)argc; // Marcar como no utilizado para evitar warnings
  (void)argv; // Marcar como no utilizado para evitar warnings

  int game_shm_fd;
  game *game_state;
  semaphore_struct *game_semaphores;

  // --- Conexión a Memoria Compartida ---
  game_shm_fd = shm_open(SHM_STATE, O_RDONLY, 0666);
  if (game_shm_fd == -1) {
    perror("view: shm_open(game)");
    return EXIT_FAILURE;
  }

  // Obtener tamaño total con fstat y mapear completo
  struct stat sb;
  if (fstat(game_shm_fd, &sb) == -1) {
    perror("view: fstat");
    close(game_shm_fd);
    return EXIT_FAILURE;
  }
  size_t game_size = sb.st_size;
  game_state = mmap(NULL, game_size, PROT_READ, MAP_SHARED, game_shm_fd, 0);
  if (game_state == MAP_FAILED) {
    perror("view: mmap(game)");
    close(game_shm_fd);
    return EXIT_FAILURE;
  }
  close(game_shm_fd);

  // --- Conexión a Semáforos ---
  // Abrir semáforos nombrados (los crea master)
  game_semaphores = open_semaphore_memory();
  if (!game_semaphores) {
    munmap(game_state, game_size);
    return EXIT_FAILURE;
  }

  // --- Inicialización de ncurses ---
  init_ncurses();
  init_colors();

  int stats_height =
      game_state->cantPlayers + 4; // Altura para la tabla de estadísticas
  int board_height = game_state->height + 2;
  int board_width = game_state->width * 2 + 2; // ancho acorde al espaciado

  // Calcular ancho requerido para estadísticas para que no se trunque
  const int W_JUG = 3, W_PID = 5, W_ID = 3, W_NOM = 12;
  const int W_PUN = 7, W_VAL = 7, W_INV = 9, W_EST = 9;
  int stats_content_width =
      (W_JUG + W_PID + W_ID + W_NOM + W_PUN + W_VAL + W_INV + W_EST) +
      7 * 3;                                 // 7 separadores " | "
  int stats_width = stats_content_width + 4; // margen + bordes

  int win_width = board_width;
  if (stats_width > win_width)
    win_width = stats_width;

  WINDOW *stats_win = newwin(stats_height, win_width, 0, 0);
  WINDOW *board_win = newwin(board_height, win_width, stats_height, 0);

  // --- Bucle Principal de la Vista ---
  while (1) {
    if (sem_wait(&game_semaphores->A) == -1)
      break; // Esperar señal del master

    if (game_state->ended) {
      // Notificar y salir
      sem_post(&game_semaphores->B);
      break;
    }

    werase(stats_win);
    werase(board_win);

    draw_stats(stats_win, game_state);
    draw_board(board_win, game_state);

    wrefresh(stats_win);
    wrefresh(board_win);

    sem_post(&game_semaphores->B); // Avisar al master que se terminó de dibujar
  }

  // --- Limpieza ---
  // Destruir ventanas antes de finalizar ncurses para permitir que libncurses libere recursos asociados
  delwin(stats_win);
  delwin(board_win);
  cleanup_ncurses();
  munmap(game_state, game_size);
  close_semaphore_memory(game_semaphores);

  printf("Juego terminado. La vista se ha cerrado.\n");

  return EXIT_SUCCESS;
}

void init_ncurses() {
  if (getenv("TERM") == NULL) {
    setenv("TERM", "xterm-256color", 1);
  }
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  timeout(100); // No esperar por input
}

void init_colors() {
  start_color();
  // Definir pares de colores para cada jugador (fondo negro)
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(6, COLOR_CYAN, COLOR_BLACK);
  init_pair(7, COLOR_WHITE, COLOR_BLACK);
  init_pair(8, COLOR_RED,
            COLOR_BLACK); // Repetir colores si hay más de 7 jugadores
  init_pair(9, COLOR_GREEN, COLOR_BLACK);
}

// lastMove eliminado del estado compartido

void draw_stats(WINDOW *win, game *game_state) {
  box(win, 0, 0);
  // Definir anchos de columna
  const int W_JUG = 3, W_PID = 5, W_ID = 3, W_NOM = 12;
  const int W_PUN = 7, W_VAL = 7, W_INV = 9, W_EST = 9;

  // Encabezado alineado
  mvwprintw(win, 1, 2, "%-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s",
            W_JUG, "Jug", W_PID, "PID", W_ID, "ID", W_NOM, "Nombre", W_PUN,
            "Puntaje", W_VAL, "V\xC3\xA1lidos", W_INV, "Inv\xC3\xA1lidos",
            W_EST, "Estado");

  // Separador
  char sep[256];
  int pos = 0;
  /* const char *cols[] = {"", "", "", "", "", "", "", ""}; */
  int widths[] = {W_JUG, W_PID, W_ID, W_NOM, W_PUN, W_VAL, W_INV, W_EST};
  for (int c = 0; c < 8; c++) {
    if (c > 0) {
      sep[pos++] = ' ';
      sep[pos++] = '|';
      sep[pos++] = ' ';
    }
    for (int k = 0; k < widths[c]; k++)
      sep[pos++] = '-';
  }
  sep[pos] = '\0';
  mvwprintw(win, 2, 2, "%s", sep);

  // Filas
  for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
    wattron(win, COLOR_PAIR(i + 1));
    char label = 'A' + (char)i;
    mvwprintw(win, 3 + i, 2, "%*c | %*d | %*d | %-*s | %*u | %*u | %*u | %-*s",
              W_JUG, label, W_PID, (int)game_state->players[i].pid, W_ID,
              (int)i, W_NOM, game_state->players[i].playerName, W_PUN,
              game_state->players[i].score, W_VAL,
              game_state->players[i].validMove, W_INV,
              game_state->players[i].invalidMove, W_EST,
              game_state->players[i].blocked ? "BLOQUEADO" : "ACTIVO");
    wattroff(win, COLOR_PAIR(i + 1));
  }
}

void draw_board(WINDOW *win, game *game_state) {
  box(win, 0, 0);

  // Dibujar con espaciado: cada celda ocupa 2 columnas
  for (int y = 0; y < game_state->height; y++) {
    for (int x = 0; x < game_state->width; x++) {
      int val = game_state->startBoard[y * game_state->width + x];
      int drawx = 1 + x * 2;
      if (val > 0) {
        char ch = (char)('0' + (val % 10));
        mvwaddch(win, y + 1, drawx, ch);
      } else {
        // Cuerpo de jugador; el valor negativo indica jugador (-(id+1))
        int pid = (-val);
        if (pid < (int)game_state->cantPlayers) {
          wattron(win, COLOR_PAIR(pid + 1));
          char label = playersChar[pid];
          mvwaddch(win, y + 1, drawx, label);
          wattroff(win, COLOR_PAIR(pid + 1));
        } else {
          mvwaddch(win, y + 1, drawx, '?');
        }
      }
    }
  }
  for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
    if (!game_state->players[i].blocked) {
      wattron(win, COLOR_PAIR(i + 1) | A_BOLD);
      char label = '*';
      mvwaddch(win, game_state->players[i].qy + 1,
               1 + game_state->players[i].qx * 2,
               label); // Cabeza del jugador como letra
      wattroff(win, COLOR_PAIR(i + 1) | A_BOLD);
    }
  }
}

void cleanup_ncurses() { endwin(); }
