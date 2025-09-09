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

// Aggressive player algorithm - prefers moves toward center and challenges other players
int choose_aggressive_move(game *game_state, int player_id) {
    player *me = &game_state->players[player_id];
    int my_x = me->qx;
    int my_y = me->qy;
    int width = game_state->width;
    int height = game_state->height;
    int center_x = width / 2;
    int center_y = height / 2;
    
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
        
        // Board check: forbid bodies (negative). Allow 0 or positive cells
        int board_value = game_state->startBoard[new_y * width + new_x];
        if (board_value < 0) {
            continue;
        }
        
        // Aggressive scoring: prefer center positions and challenging other players
        int score = 0;
        
        // Distance to center (smaller is better for aggressive)
        int center_distance = abs(new_x - center_x) + abs(new_y - center_y);
        score += (width + height - center_distance); // Prefer being closer to center
        
        // Bonus for being in the exact center
        if (new_x == center_x && new_y == center_y) {
            score += 20;
        }
        
        // Look for opportunities to challenge other players
        for (int i = 0; i < (int)game_state->cantPlayers; i++) {
            if (i != player_id && !game_state->players[i].blocked) {
                int dist_x = abs(new_x - game_state->players[i].qx);
                int dist_y = abs(new_y - game_state->players[i].qy);
                int distance = dist_x + dist_y;
                
                // Prefer getting closer to other players (aggressive behavior)
                if (distance <= 2) {
                    score += (3 - distance) * 8; // Bonus for being close
                }
                
                // Special bonus for potentially blocking other players
                if (distance == 1) {
                    score += 15;
                }
            }
        }
        
        // Prefer moves that open up more possibilities (avoid corners/edges)
        int edge_distance = new_x;
        if (width - 1 - new_x < edge_distance) edge_distance = width - 1 - new_x;
        if (new_y < edge_distance) edge_distance = new_y;
        if (height - 1 - new_y < edge_distance) edge_distance = height - 1 - new_y;
        
        score += edge_distance * 2; // Prefer being away from edges
        
        // Count available moves from the new position (mobility)
        int mobility = 0;
        for (int check_dir = 0; check_dir < 8; check_dir++) {
            int check_x = new_x + dx[check_dir];
            int check_y = new_y + dy[check_dir];
            
            if (check_x >= 0 && check_x < width && check_y >= 0 && check_y < height) {
                int check_occupied = 0;
                for (int j = 0; j < (int)game_state->cantPlayers; j++) {
                    if (game_state->players[j].qx == check_x && 
                        game_state->players[j].qy == check_y &&
                        !game_state->players[j].blocked) {
                        check_occupied = 1;
                        break;
                    }
                }
                
                if (!check_occupied && game_state->startBoard[check_y * width + check_x] >= 0) {
                    mobility++;
                }
            }
        }
        score += mobility * 3; // Prefer positions with more movement options
        
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
        fprintf(stderr, "player2: Failed to open shared memory or semaphores\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL) ^ getpid()); // Seed for random moves

    // Determine own player_id by PID (retry briefly)
    int player_id = -1;
    pid_t self = getpid();
    for (int attempt = 0; attempt < 200 && player_id < 0; attempt++) {
        if (acquire_read_access(sem_state) == 0) {
            for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
                if (game_state->players[i].pid == self) { player_id = (int)i; break; }
            }
            release_read_access(sem_state);
        }
        //if (player_id < 0) { struct timespec ts = {0, 1000000}; nanosleep(&ts, NULL); }
    }
    if (player_id < 0) {
        close_semaphore_memory(sem_state);
        size_t sz = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
        close_shared_memory(game_state, sz);
        return EXIT_FAILURE;
    }

    int prev_count = -1;
    while (!game_state->ended && !game_state->players[player_id].blocked) {
        // Wait until master grants this player's turn
        if (wait_for_turn(sem_state, player_id) == -1) {
            break;
        }
        if (acquire_read_access(sem_state) == -1) {
            break;
        }
        int count = (int)(game_state->players[player_id].validMove + game_state->players[player_id].invalidMove);
        int skip_write = (count == prev_count);
        prev_count = count;

        int move_direction = choose_aggressive_move(game_state, player_id);
        if (release_read_access(sem_state) == -1) {
            break;
        }

        if (move_direction == -1) {
            // No hay movimientos posibles: cerrar stdout para enviar EOF
            close(STDOUT_FILENO);
            break;
        }

        if (!skip_write) {
            unsigned char b = (unsigned char)move_direction;
            ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
            if (bytes_written != 1) {
                perror("player2 write");
                break;
            }
        }
        //{ struct timespec ts = {0, 2000000}; nanosleep(&ts, NULL); }
    }

    // No imprimir mensajes de salida para no interferir con la vista

    // Clean up
    close_semaphore_memory(sem_state);
    size_t game_size = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
    close_shared_memory(game_state, game_size);
    // No cerrar stdout manualmente
    
    return EXIT_SUCCESS;
}
