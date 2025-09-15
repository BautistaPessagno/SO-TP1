// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/config.h"
#include <getopt.h>
#include <time.h>

// Parámetros de configuración globales
int delay =
    200; // milisegundos que espera el máster cada vez que se imprime el estado
int timeout =
    10; // timeout en segundos para recibir solicitudes de movimientos válidos
int seed =
    0; // semilla utilizada para la generación del tablero (0 = time(NULL))
char *view_path = NULL; // ruta del binario de la vista

void print_usage(const char *program_name) {
  fprintf(stderr,
          "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] "
          "[-v view] -p player_cente [player_cente ...]\n",
          program_name);
  fprintf(stderr,
          "  -w width    Ancho del tablero (mínimo 10, por defecto 10)\n");
  fprintf(stderr,
          "  -h height   Alto del tablero (mínimo 10, por defecto 10)\n");
  fprintf(stderr, "  -d delay    Milisegundos que espera el máster cada vez q"
                  "ue se imprime el estado (por defecto 200)\n");
  fprintf(stderr, "  -t timeout  Timeout en segundos para recibir solicitudes "
                  "de movimientos válidos (por defecto 10)\n");
  fprintf(stderr, "  -s seed     Semilla utilizada para la generación del t"
                  "ablero (por defecto time(NULL))\n");
  fprintf(
      stderr,
      "  -v view     Ruta del binario de la vista (por defecto sin vista)\n");
  fprintf(stderr, "  -p player   Ruta/s de los binarios de los jugadores "
                  "(mínimo 1, máximo 9)\n");
  fprintf(stderr, "              Ejemplo: -p player_cente player_cente\n");
}

int parse_arguments(int argc, char *argv[], int *width, int *height,
                    int *num_players, char *player_executables[]) {
  // Parámetros por defecto
  *width = 10;
  *height = 10;
  *num_players = 0;

  // Parsear argumentos de línea de comandos con getopt
  int opt;
  while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
    switch (opt) {
    case 'w':
      *width = atoi(optarg);
      break;
    case 'h':
      *height = atoi(optarg);
      break;
    case 'd':
      delay = atoi(optarg);
      break;
    case 't':
      timeout = atoi(optarg);
      break;
    case 's':
      seed = atoi(optarg);
      break;
    case 'v':
      view_path = optarg;
      break;
    case 'p':
      if (*num_players >= 9) {
        fprintf(stderr, "Error: Máximo 9 jugadores.\n");
        return EXIT_FAILURE;
      }
      // Si no se proporciona una ruta, asumir que está en el directorio actual
      if (strchr(optarg, '/') == NULL) {
        // Construir "./<nombre>" de forma segura
        size_t name_len = strlen(optarg);
        size_t total = name_len + 3; // "./" + nombre + NUL
        char *executable_with_path = (char *)malloc(total);
        if (executable_with_path == NULL) {
          perror("malloc");
          return EXIT_FAILURE;
        }
        int written = snprintf(executable_with_path, total, "./%s", optarg);
        if (written < 0 || (size_t)written >= total) {
          fprintf(stderr, "Error: ruta de jugador demasiado larga.\n");
          free(executable_with_path);
          return EXIT_FAILURE;
        }
        player_executables[*num_players] = executable_with_path;
      } else {
        player_executables[*num_players] = optarg;
      }
      (*num_players)++;
      break;
    default:
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  // Procesar argumentos restantes como jugadores adicionales
  for (int i = optind; i < argc; i++) {
    if (*num_players >= 9) {
      fprintf(stderr, "Error: Máximo 9 jugadores.\n");
      return EXIT_FAILURE;
    }
    // Si no se proporciona una ruta, asumir que está en el directorio actual
    if (strchr(argv[i], '/') == NULL) {
      // Construir "./<nombre>" de forma segura
      size_t name_len = strlen(argv[i]);
      size_t total = name_len + 3; // "./" + nombre + NUL
      char *executable_with_path = (char *)malloc(total);
      if (executable_with_path == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
      }
      int written = snprintf(executable_with_path, total, "./%s", argv[i]);
      if (written < 0 || (size_t)written >= total) {
        fprintf(stderr, "Error: ruta de jugador demasiado larga.\n");
        free(executable_with_path);
        return EXIT_FAILURE;
      }
      player_executables[*num_players] = executable_with_path;
    } else {
      player_executables[*num_players] = argv[i];
    }
    (*num_players)++;
  }

  // Verificar que se especificó al menos un jugador
  if (*num_players == 0) {
    fprintf(stderr, "Error: Se debe especificar al menos un jugador con -p\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  return 0;
}

void validate_parameters(int width, int height, int num_players) {
  (void)num_players; // Suppress unused parameter warning
  // Validar parámetros según especificaciones
  if (width < 10 || height < 10) {
    fprintf(stderr, "Error: width y height deben ser mínimo 10.\n");
    exit(EXIT_FAILURE);
  }

  // Configurar semilla si no se especificó
  if (seed == 0) {
    seed = time(NULL);
  }
}
