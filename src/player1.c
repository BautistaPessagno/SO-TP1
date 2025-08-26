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
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Open shared memory and semaphores
    game *game_state = open_shared_memory();
    semaphore_struct *sem_state = open_semaphore_memory();

    if (!game_state || !sem_state) {
        fprintf(stderr, "player1: Failed to open shared memory or semaphores\n");
        return EXIT_FAILURE;
    }
    
    // Determine own player_id by PID (retry briefly to avoid race with master)
    int player_id = -1;
    pid_t self = getpid();
    for (int attempt = 0; attempt < 200 && player_id < 0; attempt++) { // ~200ms
        if (acquire_read_access(sem_state) == 0) {
            for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
                if (game_state->players[i].pid == self) { player_id = (int)i; break; }
            }
            release_read_access(sem_state);
        }
        if (player_id < 0) usleep(1000);
    }
    if (player_id < 0) {
        close_semaphore_memory(sem_state);
        size_t sz = sizeof(game) + (game_state->width * game_state->high * sizeof(int));
        close_shared_memory(game_state, sz);
        return EXIT_FAILURE;
    }

    // Game loop
    int prev_count = -1;
    while (!game_state->ended && !game_state->players[player_id].blocked) {
        if (acquire_read_access(sem_state) == -1) {
            break;
        }
        int count = (int)(game_state->players[player_id].validMove + game_state->players[player_id].invalidMove);
        int skip_write = (count == prev_count);
        prev_count = count;

        int move_direction = choose_conservative_move(game_state, player_id);

        if (release_read_access(sem_state) == -1) {
            break;
        }

        if (!skip_write && move_direction != -1) {
            unsigned char b = (unsigned char)move_direction;
            ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
            if (bytes_written != 1) {
                perror("player1 write");
                break;
            }
        }
        usleep(2000);
    }

    // No imprimir mensajes de salida para no interferir con la vista

    // Clean up
    close_semaphore_memory(sem_state);
    size_t game_size = sizeof(game) + (game_state->width * game_state->high * sizeof(int));
    close_shared_memory(game_state, game_size);
    // No cerrar stdout manualmente

    return EXIT_SUCCESS;
}
