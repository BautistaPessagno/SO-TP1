#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// copio los structs dados por la catedra

typedef struct {
  char playerName[16];      // Nombre del jugador
  unsigned int score;       // Puntaje
  unsigned int invalidMove; // Cantidad de solicitudes de movimientos inválidas
                            // realizadas
  unsigned int
      validMove; // Cantidad de solicitudes de movimientos válidas realizadas
  unsigned short qx, qy; // Coordenadas x e y en el tablero
  pid_t pid;             // Identificador de proceso
  char blocked;          // Indica si el jugador está bloqueado
} player;

typedef struct {
  unsigned short width;     // Ancho del tablero
  unsigned short high;      // Alto del tablero
  unsigned int cantPlayers; // Cantidad de jugadores
  player players[9];        // Lista de jugadores
  char ended;               // Indica si el juego se ha terminado
  int startBoard[]; // Puntero al comienzo del tablero. fila-0, fila-1, ...,
                    // fila-n-1
} game;

/* Sincronización */
/* Para la sincronización entre los procesos involucrados, se utilizan los
 * siguientes semáforos */
/* anónimos (sem_ overview(7)). En este caso, los nombres se reemplazaron por
 * letras mayúsculas ya */
/* que es necesario identificarlos para explicar su uso, sin embargo, asignarles
 * nombres apropiados */
/* también es parte de la evaluación. La siguiente estructura se almacena en una
 * memoria compartida */
// cuyo nombre es “/game_sync” (shm_overview(7)).

typedef struct {
  sem_t A;        // El máster le indica a la vista que hay cambios por imprimir
  sem_t B;        // La vista le indica al máster que terminó de imprimir
  sem_t C;        // Mutex para evitar inanición del máster al acceder al estado
  sem_t D;        // Mutex para el estado del juego
  sem_t E;        // Mutex para la siguiente variable
  unsigned int F; // Cantidad de jugadores leyendo el estado
  sem_t G[9];     // Le indican a cada jugador que puede enviar 1 movimiento
} semaphore;      // semaphore

#include <fcntl.h>
#include <sys/mman.h>

// prueba de memoria compartida

#define SIZE 1024
#define NAME "/mimemoria"

int main() {
  int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666); // Crear objeto
  ftruncate(fd, SIZE);                             // Ajustar tamaño
  char *ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

  sprintf(ptr, "Hola desde PID %d", getpid());

  printf("Escrito en memoria compartida POSIX\n");

  munmap(ptr, SIZE);
  close(fd);
  return 0;
}
