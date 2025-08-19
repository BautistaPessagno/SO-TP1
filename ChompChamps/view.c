
// codigo prueba de ncurses para entender como funciona
#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>

static int is_number(const char *s) {
  if (!s || !*s)
    return 0;
  for (; *s; ++s)
    if (!isdigit((unsigned char)*s))
      return 0;
  return 1;
}

// Calcula el tamaño real en celdas de la grilla en caracteres:
// Por cada celda hay 1 (alto) y 1 (ancho) + líneas divisorias.
// Altura total = filas*2 + 1 (líneas horizontales+intersecciones)
// Ancho total  = columnas*2 + 1 (líneas verticales+intersecciones)
static void draw_board(int rows, int cols, int starty, int startx) {
  int H = rows * 2 + 1;
  int W = cols * 2 + 1;

  // Recorremos cada punto de la "malla" y colocamos el char adecuado.
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int sy = starty + y;
      int sx = startx + x;

      bool on_hline = (y % 2 == 0);
      bool on_vline = (x % 2 == 0);

      chtype ch = ' ';

      if (on_hline && on_vline) {
        // Intersecciones y esquinas
        if (y == 0 && x == 0)
          ch = ACS_ULCORNER;
        else if (y == 0 && x == W - 1)
          ch = ACS_URCORNER;
        else if (y == H - 1 && x == 0)
          ch = ACS_LLCORNER;
        else if (y == H - 1 && x == W - 1)
          ch = ACS_LRCORNER;
        else if (y == 0)
          ch = ACS_TTEE;
        else if (y == H - 1)
          ch = ACS_BTEE;
        else if (x == 0)
          ch = ACS_LTEE;
        else if (x == W - 1)
          ch = ACS_RTEE;
        else
          ch = ACS_PLUS;
      } else if (on_hline && !on_vline) {
        ch = ACS_HLINE;
      } else if (!on_hline && on_vline) {
        ch = ACS_VLINE;
      } else {
        ch = ' '; // interior de la celda
      }

      mvaddch(sy, sx, ch);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3 || !is_number(argv[1]) || !is_number(argv[2])) {
    fprintf(stderr, "Uso: %s <filas> <columnas>\n", argv[0]);
    return 1;
  }

  int filas = atoi(argv[1]);
  int cols = atoi(argv[2]);
  if (filas <= 0 || cols <= 0) {
    fprintf(stderr, "Las dimensiones deben ser positivas.\n");
    return 1;
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  // Calcular tamaño requerido en caracteres
  int req_h = filas * 2 + 1;
  int req_w = cols * 2 + 1;

  int maxy, maxx;
  getmaxyx(stdscr, maxy, maxx);

  if (req_h > maxy || req_w > maxx) {
    endwin();
    fprintf(stderr,
            "La grilla (%dx%d chars) no entra en tu terminal (%dx%d).\n"
            "Agrandá la terminal o probá con menos filas/columnas.\n",
            req_h, req_w, maxy, maxx);
    return 1;
  }

  // Centrar
  int starty = (maxy - req_h) / 2;
  int startx = (maxx - req_w) / 2;

  clear();
  draw_board(filas, cols, starty, startx);

  mvprintw(starty + req_h + (starty + req_h < maxy ? 1 : 0), startx,
           "Tablero %dx%d - presiona 'q' para salir", filas, cols);
  refresh();

  int ch;
  while ((ch = getch()) != 'q' && ch != 'Q') {
    // Podés agregar interacción acá (mover cursor, resaltar celdas, etc.)
  }

  endwin();
  return 0;
}
