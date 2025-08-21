#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"

// Prototipos de funciones
void init_ncurses();
void init_colors();
void draw_stats(WINDOW *win, game *game_state);
void draw_board(WINDOW *win, game *game_state);
void cleanup_ncurses();

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
    if (fstat(game_shm_fd, &sb) == -1) { perror("view: fstat"); close(game_shm_fd); return EXIT_FAILURE; }
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

    int stats_height = game_state->cantPlayers + 4; // Altura para la tabla de estadísticas
    int board_height = game_state->high + 2;
    int board_width = game_state->width * 2 + 2; // ancho acorde al espaciado

    WINDOW *stats_win = newwin(stats_height, board_width, 0, 0);
    WINDOW *board_win = newwin(board_height, board_width, stats_height, 0);

    // --- Bucle Principal de la Vista ---
    while (1) {
        if (sem_wait(game_semaphores->A) == -1) break; // Esperar señal del master

        if (game_state->ended) {
            // Notificar y salir
            sem_post(game_semaphores->B);
            break;
        }

        werase(stats_win);
        werase(board_win);

        draw_stats(stats_win, game_state);
        draw_board(board_win, game_state);

        wrefresh(stats_win);
        wrefresh(board_win);

        sem_post(game_semaphores->B); // Avisar al master que se terminó de dibujar
    }

    // --- Limpieza ---
    cleanup_ncurses();
    delwin(stats_win);
    delwin(board_win);
    munmap(game_state, game_size);
    close_semaphore_memory(game_semaphores);

    printf("Juego terminado. La vista se ha cerrado.\n");

    return EXIT_SUCCESS;
}

void init_ncurses() {
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
    init_pair(8, COLOR_RED, COLOR_BLACK); // Repetir colores si hay más de 7 jugadores
    init_pair(9, COLOR_GREEN, COLOR_BLACK);
}

static const char* dir_to_str(int d) {
    switch (d) {
        case 0: return "N "; case 1: return "NE"; case 2: return "E "; case 3: return "SE";
        case 4: return "S "; case 5: return "SW"; case 6: return "W "; case 7: return "NW";
        default: return "- ";
    }
}

void draw_stats(WINDOW *win, game *game_state) {
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "Jug | Nombre     | Puntaje | Válidos | Inválidos | Último | Bloq");
    mvwprintw(win, 2, 2, "----+------------+---------+---------+-----------+--------+-----");

    for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
        wattron(win, COLOR_PAIR(i + 1));
        mvwprintw(win, 3 + i, 2, "%3u | %-10s | %7u | %7u | %9u | %-6s | %s",
                  i,
                  game_state->players[i].playerName,
                  game_state->players[i].score,
                  game_state->players[i].validMove,
                  game_state->players[i].invalidMove,
                  dir_to_str(game_state->players[i].lastMove),
                  game_state->players[i].blocked ? "SI" : "NO");
        wattroff(win, COLOR_PAIR(i + 1));
    }
}

void draw_board(WINDOW *win, game *game_state) {
    box(win, 0, 0);
    
    // Dibujar con espaciado: cada celda ocupa 2 columnas
    for (int y = 0; y < game_state->high; y++) {
        for (int x = 0; x < game_state->width; x++) {
            int val = game_state->startBoard[y * game_state->width + x];
            int drawx = 1 + x * 2;
            if (val > 0) {
                char ch = (char)('0' + (val % 10));
                mvwaddch(win, y + 1, drawx, ch);
            } else if (val < 0) {
                // Cuerpo de una snake; el valor negativo indica jugador (-(id+1))
                int pid = (-val) - 1;
                if (pid >= 0 && pid < (int)game_state->cantPlayers) {
                    wattron(win, COLOR_PAIR(pid + 1));
                    mvwaddch(win, y + 1, drawx, 'o');
                    wattroff(win, COLOR_PAIR(pid + 1));
                } else {
                    mvwaddch(win, y + 1, drawx, 'o');
                }
            } else {
                mvwaddch(win, y + 1, drawx, '.');
            }
        }
    }
    for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
        if (!game_state->players[i].blocked) {
            wattron(win, COLOR_PAIR(i + 1) | A_BOLD);
            mvwaddch(win, game_state->players[i].qy + 1, 1 + game_state->players[i].qx * 2, '@'); // Cabeza del jugador
            wattroff(win, COLOR_PAIR(i + 1) | A_BOLD);
        }
    }
}

void cleanup_ncurses() {
    endwin();
}
