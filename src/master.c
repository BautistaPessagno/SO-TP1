#include "include/game.h"
#include "include/game_semaphore.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

// Definiciones y estructura del juego vienen de include/game.h

// La estructura de semáforos ahora está en include/game_semaphore.h

// Variables globales
int game_shm_fd, sem_shm_fd;
game *game_state;
semaphore_struct *game_semaphores;
int player_pipes[9][2]; // Pipes para comunicación con jugadores

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
  game_state->high = height;
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

// Función para inicializar el tablero con valores aleatorios
void initialize_board() {
  srand(time(NULL));

  // Llenar tablero con valores aleatorios (1-9)
  for (int i = 0; i < game_state->width * game_state->high; i++) {
    game_state->startBoard[i] = (rand() % 9) + 1;
  }

  // Limpiar posiciones de jugadores (valor 0)
  for (int i = 0; i < (int)game_state->cantPlayers; i++) {
    int pos = game_state->players[i].qy * game_state->width +
              game_state->players[i].qx;
    game_state->startBoard[pos] = 0;
  }
}

// Función para inicializar jugadores
void initialize_players() {
  char player_names[][16] = {"Player1", "Player2", "Player3",
                             "Player4", "Player5", "Player6",
                             "Player7", "Player8", "Player9"};

  for (int i = 0; i < (int)game_state->cantPlayers; i++) {
    strcpy(game_state->players[i].playerName, player_names[i]);
    game_state->players[i].score = 0;
    game_state->players[i].invalidMove = 0;
    game_state->players[i].validMove = 0;
    game_state->players[i].blocked = 0;
    // lastMove eliminado del estado compartido

    // Posición inicial aleatoria
    do {
      game_state->players[i].qx = rand() % game_state->width;
      game_state->players[i].qy = rand() % game_state->high;
    } while (/* verificar que no haya otro jugador en esa posición */ 0);

    // Verificar que no se solape con otros jugadores
    for (int j = 0; j < i; j++) {
      if (game_state->players[i].qx == game_state->players[j].qx &&
          game_state->players[i].qy == game_state->players[j].qy) {
        i--; // Reintentar con esta posición
        break;
      }
    }
  }
}

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

// Direcciones: 0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW
static const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

static int is_inside(int x, int y) {
  return x >= 0 && x < (int)game_state->width && y >= 0 &&
         y < (int)game_state->high;
}

static int is_occupied(int x, int y, int self_id) {
  // Ocupado por cuerpo
  int cell = game_state->startBoard[y * game_state->width + x];
  if (cell < 0)
    return 1;
  // Ocupado por cabeza de algún jugador
  for (int j = 0; j < (int)game_state->cantPlayers; j++) {
    if (j == self_id)
      continue;
    if (game_state->players[j].qx == x && game_state->players[j].qy == y &&
        !game_state->players[j].blocked)
      return 1;
  }
  return 0;
}

static void apply_player_move(int pid, int direction) {
  if (direction < 0 || direction > 7) {
    game_state->players[pid].invalidMove++;
    return;
  }
  int x = game_state->players[pid].qx;
  int y = game_state->players[pid].qy;
  int nx = x + dx[direction];
  int ny = y + dy[direction];
  if (!is_inside(nx, ny) || is_occupied(nx, ny, pid)) {
    game_state->players[pid].invalidMove++;
    // No bloquear de inmediato: permitir que intente otro movimiento en el
    // próximo turno game_state->players[pid].blocked = 1;
    return;
  }
  // Dejar cuerpo en la celda actual
  game_state->startBoard[y * game_state->width + x] = -(pid + 1);
  // Puntuar por la celda destino si tiene valor positivo
  int dest_idx = ny * game_state->width + nx;
  int cell_val = game_state->startBoard[dest_idx];
  if (cell_val > 0) {
    game_state->players[pid].score += (unsigned int)cell_val;
  }
  // Mover cabeza
  game_state->players[pid].qx = (unsigned short)nx;
  game_state->players[pid].qy = (unsigned short)ny;
  // lastMove eliminado del estado compartido
  // Limpiar la celda destino para que no muestre puntaje (se verá la cabeza por
  // encima)
  game_state->startBoard[dest_idx] = 0;
  game_state->players[pid].validMove++;
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
    execve("./view", args, environ);
    perror("execve view");
    exit(EXIT_FAILURE);
  }

  return view_pid;
}

// Función para crear procesos de jugadores
pid_t create_player_process(int player_id, const char *player_executable,
                            int pipe_fd) {
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

    // Pasar ancho y alto como argumentos, alineado con testPlayer
    char width_str[16], height_str[16];
    snprintf(width_str, sizeof(width_str), "%d", (int)game_state->width);
    snprintf(height_str, sizeof(height_str), "%d", (int)game_state->high);

    char *args[] = {(char *)player_executable, width_str, height_str, NULL};
    execve(player_executable, args, environ);
    perror("execve player");
    exit(EXIT_FAILURE);
  }

  return player_pid;
}

int main(int argc, char *argv[]) {
  // Limpiar cualquier segmento de memoria compartida previo
  shm_unlink(SHM_STATE);
  shm_unlink(SHM_SEM);

  // Parámetros por defecto
  int width = 10, height = 10;
  int num_players;
  char *player_executables[9];

  // Parsear argumentos de línea de comandos
  if (argc < 3) {
    fprintf(stderr,
            "Uso: %s <width> <height> [player_executable_1] "
            "[player_executable_2] ...\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  width = atoi(argv[1]);
  height = atoi(argv[2]);

  if (argc >= 4) {
    num_players = argc - 3;
    if (num_players > 9) {
      fprintf(stderr, "Error: Máximo 9 jugadores.\n");
      return EXIT_FAILURE;
    }
    for (int i = 0; i < num_players; i++) {
      if (strchr(argv[i + 3], '/') == NULL) {
        // Si no se proporciona una ruta, asumir que está en el directorio
        // actual
        char *executable_with_path = malloc(strlen(argv[i + 3]) + 3);
        if (executable_with_path == NULL) {
          perror("malloc");
          return EXIT_FAILURE;
        }
        sprintf(executable_with_path, "./%s", argv[i + 3]);
        player_executables[i] = executable_with_path;
      } else {
        player_executables[i] = argv[i + 3];
      }
    }
  } else {
    // Configuración por defecto si no se especifican jugadores
    printf("Usando configuración de jugadores por defecto: ./player1 y "
           "./player2\n");
    num_players = 2;
    player_executables[0] = "./player1";
    player_executables[1] = "./player2";
  }

  // Validar parámetros
  if (width < 5 || width > 50 || height < 5 || height > 50) {
    fprintf(stderr, "Parámetros inválidos.\n");
    fprintf(stderr, "width/height: 5-50\n");
    return EXIT_FAILURE;
  }

  printf("Iniciando ChompChamps - %dx%d tablero, %d jugadores\n", width, height,
         num_players);

  // Crear memoria compartida
  if (create_game_shared_memory(width, height, num_players) == -1) {
    return EXIT_FAILURE;
  }

  if (create_semaphore_shared_memory() == -1) {
    return EXIT_FAILURE;
  }

  // Inicializar jugadores y tablero
  initialize_players();
  initialize_board();

  // Crear pipes de comunicación
  if (create_player_pipes() == -1) {
    return EXIT_FAILURE;
  }

  // Crear procesos de jugadores
  pid_t player_pids[9];
  for (int i = 0; i < num_players; i++) {
    player_pids[i] =
        create_player_process(i, player_executables[i], player_pipes[i][1]);

    if (player_pids[i] == -1) {
      return EXIT_FAILURE;
    }
    // Registrar PID del jugador en el estado compartido (protegido por D)
    sem_wait(&game_semaphores->D);
    game_state->players[i].pid = player_pids[i];
    sem_post(&game_semaphores->D);
    // Cerrar el extremo de escritura del pipe en el padre
    close(player_pipes[i][1]);
  }

  // Crear proceso de la vista
  pid_t view_pid = create_view_process(width, height);
  if (view_pid == -1) {
    return EXIT_FAILURE;
  }

  printf("Todos los procesos creados. Iniciando juego...\n");

  // Señalar a la vista que puede empezar
  sem_post(&game_semaphores->A);

  // Loop principal del juego
  while (!game_state->ended) {
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
      for (int i = 0; i < num_players; i++) {
        if (game_state->players[i].blocked)
          continue;
        if (pfds[i].revents & POLLIN) {
          unsigned char move_byte;
          ssize_t r = read(player_pipes[i][0], &move_byte, 1);
          if (r == 1) {
            int direction = (int)move_byte;
            // Exclusión mutua de escritura del estado del juego (RW-lock)
            sem_wait(&game_semaphores->D);
            apply_player_move(i, direction);
            sem_post(&game_semaphores->D);
          } else if (r == 0) {
            // EOF: jugador sin más movimientos -> bloquear
            game_state->players[i].blocked = 1;
          }
        }
        if (pfds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
          // Pipe cerrado o error: considerar bloqueado
          game_state->players[i].blocked = 1;
        }
      }
    }

    // Señalar a la vista que actualice
    sem_post(&game_semaphores->A);

    // Esperar que la vista termine de actualizar
    sem_wait(&game_semaphores->B);

    // Pequeño respiro para no saturar CPU y dar tiempo a jugadores
    usleep(50000); // 50ms

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
      // Despertar la vista para que pueda leer ended y salir
      sem_post(&game_semaphores->A);
    }
  }

  // Calcular ganador: mayor puntaje; en empate, menor validMove
  int winner = -1;
  unsigned int best_score = 0;
  unsigned int best_valid = 0xFFFFFFFFu;
  for (int i = 0; i < num_players; i++) {
    unsigned int s = game_state->players[i].score;
    unsigned int v = game_state->players[i].validMove;
    if (winner == -1 || s > best_score || (s == best_score && v < best_valid)) {
      winner = i;
      best_score = s;
      best_valid = v;
    }
  }

  // Esperar que terminen todos los procesos
  for (int i = 0; i < num_players; i++) {
    waitpid(player_pids[i], NULL, 0);
  }
  waitpid(view_pid, NULL, 0);

  printf("Resultados finales:\n");
  for (int i = 0; i < num_players; i++) {
    printf(
        "Jugador %c | Puntaje: %u | Válidos: %u | Inválidos: %u | Estado: %s\n",
        'A' + i, game_state->players[i].score, game_state->players[i].validMove,
        game_state->players[i].invalidMove,
        game_state->players[i].blocked ? "BLOQ" : "ACTIVO");
  }
  if (winner >= 0) {
    printf("Ganador: Jugador %c (score=%u, validos=%u)\n", 'A' + winner,
           best_score, best_valid);
  } else {
    printf("Sin ganador.\n");
  }

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

  printf("Master terminado.\n");
  return EXIT_SUCCESS;
}
