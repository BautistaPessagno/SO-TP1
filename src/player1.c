#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"

// Directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// Conservative player algorithm - prefers moves toward walls/edges
int choose_conservative_move(game *game_state, int player_id) {
    player *me = &game_state->players[player_id];
    int my_x = me->qx;
    int my_y = me->qy;
    int width = game_state->width;
    int height = game_state->high;
    
    int best_direction = -1;
    int best_score = -1000;
    
    // Check all 8 directions
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        
        // Check if move is valid (within bounds)
        if (new_x < 0 || new_x >= width || new_y < 0 || new_y >= height) {
            continue;
        }
        
        // Check if cell is occupied by another player
        int occupied = 0;
        for (int i = 0; i < (int)game_state->cantPlayers; i++) {
            if (i != player_id && 
                game_state->players[i].qx == new_x && 
                game_state->players[i].qy == new_y &&
                !game_state->players[i].blocked) {
                occupied = 1;
                break;
            }
        }
        
        if (occupied) {
            continue;
        }
        
        // Check board: only bodies (negative) are forbidden; positive cells are allowed (score)
        int board_value = game_state->startBoard[new_y * width + new_x];
        if (board_value < 0) {
            continue;
        }
        
        // Conservative scoring: prefer positions near edges
        int score = 0;
        
        // Distance to nearest edge (smaller is better for conservative)
        int edge_distance = new_x;
        if (width - 1 - new_x < edge_distance) edge_distance = width - 1 - new_x;
        if (new_y < edge_distance) edge_distance = new_y;
        if (height - 1 - new_y < edge_distance) edge_distance = height - 1 - new_y;
        
        score += (10 - edge_distance); // Prefer being closer to edges
        
        // Avoid getting too close to other players
        for (int i = 0; i < (int)game_state->cantPlayers; i++) {
            if (i != player_id && !game_state->players[i].blocked) {
                int dist_x = abs(new_x - game_state->players[i].qx);
                int dist_y = abs(new_y - game_state->players[i].qy);
                int distance = dist_x + dist_y;
                if (distance < 3) {
                    score -= (3 - distance) * 5; // Penalty for being too close
                }
            }
        }
        
        // Prefer corners
        if ((new_x == 0 || new_x == width - 1) && (new_y == 0 || new_y == height - 1)) {
            score += 15;
        }
        
        if (score > best_score) {
            best_score = score;
            best_direction = dir;
        }
    }
    
    return best_direction;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <player_id> <pipe_fd>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int player_id = atoi(argv[1]);
    int pipe_fd = atoi(argv[2]);
    
    // Open shared memory and semaphores
    game *game_state = open_shared_memory();
    semaphore_struct *sem_state = open_semaphore_memory();

    if (!game_state || !sem_state) {
        fprintf(stderr, "Player %d: Failed to open shared memory or semaphores\n", player_id);
        return EXIT_FAILURE;
    }
    
    // Game loop
    while (1) {
        // Wait for turn
        if (wait_for_turn(sem_state, player_id) == -1) {
            break; // Salir si hay error en el semáforo
        }
        
        // Acquire read access to game state
        if (acquire_read_access(sem_state) == -1) {
            break;
        }

        int move_direction = choose_conservative_move(game_state, player_id);

        // Release read access
        if (release_read_access(sem_state) == -1) {
            break;
        }

        static int pipe_closed = 0;
        if (move_direction == -1) {
            // No hay movimientos válidos: cerrar pipe para enviar EOF y salir
            if (!pipe_closed) { close(pipe_fd); pipe_closed = 1; }
            break;
        } else {
            // Send move to master via pipe
            ssize_t bytes_written = write(pipe_fd, &move_direction, sizeof(move_direction));
            if (bytes_written == -1) {
                perror("write to pipe");
                break;
            }
        }
    }

    // No imprimir mensajes de salida para no interferir con la vista

    // Clean up
    close_semaphore_memory(sem_state);
    size_t game_size = sizeof(game) + (game_state->width * game_state->high * sizeof(int));
    close_shared_memory(game_state, game_size);
    // Cerrar pipe si sigue abierto
    extern int errno;
    if (pipe_fd >= 0) {
        close(pipe_fd);
    }

    return EXIT_SUCCESS;
}
