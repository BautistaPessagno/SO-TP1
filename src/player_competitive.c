#include "include/game.h"
#include "include/game_semaphore.h"
#include "include/ipc.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

// Directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// Algorithm parameters - More greedy settings
#define BFS_RADIUS 12
#define DENSITY_RADIUS 2  // Reduced from 3 - focus on immediate area
#define MOBILITY_K 4      // Reduced from 6 - less long-term planning
#define EFFICIENCY_H 2    // Reduced from 4 - shorter lookahead
#define LOOKAHEAD_DEPTH 1 // Reduced from 2 - less future planning
#define BEAM_WIDTH 2      // Reduced from 3 - narrower beam search
#define LAMBDA 0.8        // Increased from 0.7 - stronger immediate reward preference
#define BETA 0.7          // Increased from 0.5 - more density influence in efficiency

// Phase weights
typedef struct {
    double alpha1; // immediate reward
    double alpha2; // density
    double alpha3; // mobility
    double alpha4; // efficiency
    double w1;     // degree weight
    double w2;     // reach weight
} phase_weights;

// Precomputed data structures
typedef struct {
    int d_self[BFS_RADIUS + 1]; // BFS distances from current position
    int density_map[100][100];  // Local density values (assuming max 100x100 board)
    int mobility_map[100][100]; // Mobility values
    int efficiency_map[100][100]; // Efficiency values
} precomputed_data;

// Helper function to check if a cell is free and valid
static int is_cell_free(game *game_state, int player_id, int x, int y) {
    int width = game_state->width;
    int height = game_state->height;
    
    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0;
    }
    
    // Check if occupied by another player's head
    for (int i = 0; i < (int)game_state->cantPlayers; i++) {
        if (i != player_id && 
            game_state->players[i].qx == x && 
            game_state->players[i].qy == y &&
            !game_state->players[i].blocked) {
            return 0;
        }
    }
    
    // Check if it's a body (negative value)
    int cell_value = game_state->startBoard[y * width + x];
    if (cell_value < 0) {
        return 0;
    }
    
    return 1;
}


// Compute local reward density for a cell
static double compute_density(game *game_state, int player_id, int x, int y) {
    int width = game_state->width;
    (void)game_state->height; // Suppress unused warning
    double density = 0.0;
    
    for (int dy_offset = -DENSITY_RADIUS; dy_offset <= DENSITY_RADIUS; dy_offset++) {
        for (int dx_offset = -DENSITY_RADIUS; dx_offset <= DENSITY_RADIUS; dx_offset++) {
            int check_x = x + dx_offset;
            int check_y = y + dy_offset;
            
            if (is_cell_free(game_state, player_id, check_x, check_y)) {
                int cell_value = game_state->startBoard[check_y * width + check_x];
                int chebyshev_dist = (abs(dx_offset) > abs(dy_offset)) ? abs(dx_offset) : abs(dy_offset);
                density += cell_value * pow(LAMBDA, chebyshev_dist);
            }
        }
    }
    
    return density;
}

// Compute mobility (degree + reachable cells)
static void compute_mobility(game *game_state, int player_id, int x, int y, int *degree, int *reachable) {
    *degree = 0;
    *reachable = 0;
    
    // Count free neighbors (degree)
    for (int dir = 0; dir < 8; dir++) {
        int new_x = x + dx[dir];
        int new_y = y + dy[dir];
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            (*degree)++;
        }
    }
    
    // Compute reachable cells within K steps using flood fill
    int visited[100][100] = {0};
    int queue[10000][2];
    int front = 0, rear = 0;
    
    queue[rear][0] = x;
    queue[rear][1] = y;
    rear++;
    visited[y][x] = 1;
    *reachable = 1;
    
    int current_dist = 0;
    int cells_at_current_dist = 1;
    int cells_at_next_dist = 0;
    
    while (front < rear && current_dist < MOBILITY_K) {
        int curr_x = queue[front][0];
        int curr_y = queue[front][1];
        front++;
        cells_at_current_dist--;
        
        for (int dir = 0; dir < 8; dir++) {
            int new_x = curr_x + dx[dir];
            int new_y = curr_y + dy[dir];
            
            if (is_cell_free(game_state, player_id, new_x, new_y) && !visited[new_y][new_x]) {
                visited[new_y][new_x] = 1;
                queue[rear][0] = new_x;
                queue[rear][1] = new_y;
                rear++;
                cells_at_next_dist++;
                (*reachable)++;
            }
        }
        
        if (cells_at_current_dist == 0) {
            current_dist++;
            cells_at_current_dist = cells_at_next_dist;
            cells_at_next_dist = 0;
        }
    }
}

// Compute short-horizon efficiency using greedy rollout
static double compute_efficiency(game *game_state, int player_id, int start_x, int start_y) {
    int total_reward = 0;
    int current_x = start_x;
    int current_y = start_y;
    int visited[100][100] = {0};
    
    for (int step = 0; step < EFFICIENCY_H; step++) {
        visited[current_y][current_x] = 1;
        
        // Find best neighbor for greedy move
        int best_dir = -1;
        double best_score = -1.0;
        
        for (int dir = 0; dir < 8; dir++) {
            int new_x = current_x + dx[dir];
            int new_y = current_y + dy[dir];
            
            if (is_cell_free(game_state, player_id, new_x, new_y) && !visited[new_y][new_x]) {
                int immediate_reward = game_state->startBoard[new_y * game_state->width + new_x];
                double density = compute_density(game_state, player_id, new_x, new_y);
                double score = immediate_reward + BETA * density;
                
                if (score > best_score) {
                    best_score = score;
                    best_dir = dir;
                }
            }
        }
        
        if (best_dir == -1) break; // No valid moves
        
        current_x += dx[best_dir];
        current_y += dy[best_dir];
        total_reward += game_state->startBoard[current_y * game_state->width + current_x];
    }
    
    return (double)total_reward / EFFICIENCY_H;
}

// Determine game phase and return appropriate weights
static phase_weights get_phase_weights(game *game_state, int player_id) {
    phase_weights weights;
    
    // Calculate total moves made by all players
    int total_moves = 0;
    for (int i = 0; i < (int)game_state->cantPlayers; i++) {
        total_moves += game_state->players[i].validMove + game_state->players[i].invalidMove;
    }
    
    // Determine if we're ahead or behind
    int my_score = game_state->players[player_id].score;
    int max_other_score = 0;
    for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
        if (i != (unsigned int)player_id && (int)game_state->players[i].score > max_other_score) {
            max_other_score = game_state->players[i].score;
        }
    }
    
    int board_size = game_state->width * game_state->height;
    double progress = (double)total_moves / (board_size * 0.3); // Rough progress estimate
    
    if (progress < 0.3) {
        // Opening phase - More greedy
        weights.alpha1 = 1.5;  // Very high immediate reward
        weights.alpha2 = 0.6;  // Reduced density (less planning)
        weights.alpha3 = 0.3;  // Lower mobility (less strategic)
        weights.alpha4 = 0.2;  // Lower efficiency (less lookahead)
    } else if (progress < 0.7) {
        // Midgame phase - Still greedy
        weights.alpha1 = 1.3;  // High immediate reward
        weights.alpha2 = 0.7;  // Medium density
        weights.alpha3 = 0.4;  // Lower mobility
        weights.alpha4 = 0.3;  // Lower efficiency
    } else {
        // Endgame phase - Greedy even when ahead
        if (my_score >= max_other_score) {
            // Ahead: still chase immediate rewards
            weights.alpha1 = 1.0;  // Still high immediate reward
            weights.alpha2 = 0.5;  // Medium density
            weights.alpha3 = 0.6;  // Some mobility
            weights.alpha4 = 0.4;  // Some efficiency
        } else {
            // Behind/close: very greedy
            weights.alpha1 = 1.8;  // Very high immediate reward
            weights.alpha2 = 0.8;  // High density
            weights.alpha3 = 0.3;  // Low mobility
            weights.alpha4 = 0.2;  // Low efficiency
        }
    }
    
    weights.w1 = 0.6;  // Degree weight
    weights.w2 = 0.4;  // Reach weight
    
    return weights;
}

// Compute score for a candidate move
static double compute_move_score(game *game_state, int player_id, int x, int y, phase_weights *weights) {
    int immediate_reward = game_state->startBoard[y * game_state->width + x];
    double density = compute_density(game_state, player_id, x, y);
    
    int degree, reachable;
    compute_mobility(game_state, player_id, x, y, &degree, &reachable);
    double mobility = weights->w1 * degree + weights->w2 * reachable;
    
    double efficiency = compute_efficiency(game_state, player_id, x, y);
    
    return weights->alpha1 * immediate_reward +
           weights->alpha2 * density +
           weights->alpha3 * mobility +
           weights->alpha4 * efficiency;
}

// Lookahead with beam search
static double lookahead_score(game *game_state, int player_id, int x, int y, int depth, phase_weights *weights) {
    if (depth <= 0) {
        return compute_move_score(game_state, player_id, x, y, weights);
    }
    
    double best_score = compute_move_score(game_state, player_id, x, y, weights);
    
    // Find top BEAM_WIDTH moves from this position
    double move_scores[8];
    int valid_moves = 0;
    
    for (int dir = 0; dir < 8; dir++) {
        int new_x = x + dx[dir];
        int new_y = y + dy[dir];
        
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            move_scores[valid_moves] = compute_move_score(game_state, player_id, new_x, new_y, weights);
            valid_moves++;
        }
    }
    
    // Sort moves by score (simple bubble sort for small arrays)
    for (int i = 0; i < valid_moves - 1; i++) {
        for (int j = 0; j < valid_moves - i - 1; j++) {
            if (move_scores[j] < move_scores[j + 1]) {
                double temp = move_scores[j];
                move_scores[j] = move_scores[j + 1];
                move_scores[j + 1] = temp;
            }
        }
    }
    
    // Add lookahead bonus for top moves
    for (int i = 0; i < valid_moves && i < BEAM_WIDTH; i++) {
        best_score += move_scores[i] * 0.1; // Small bonus for good future moves
    }
    
    return best_score;
}

// Check if player is in a dangerous position (near edges, low mobility)
static int is_dangerous_position(game *game_state, int player_id, int x, int y) {
    int width = game_state->width;
    int height = game_state->height;
    
    // Check if near edges (within 2 cells)
    if (x < 2 || x >= width - 2 || y < 2 || y >= height - 2) {
        return 1;
    }
    
    // Check mobility - if very low, it's dangerous
    int degree, reachable;
    compute_mobility(game_state, player_id, x, y, &degree, &reachable);
    if (degree < 3 || reachable < 5) {
        return 1;
    }
    
    return 0;
}

// Emergency escape: try to find any path away from current position
static int emergency_escape(game *game_state, int player_id, int my_x, int my_y) {
    // Try to move towards center of board
    int width = game_state->width;
    int height = game_state->height;
    int center_x = width / 2;
    int center_y = height / 2;
    
    int best_dir = -1;
    int best_distance = 1000; // Large initial distance
    
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            // Calculate distance to center
            int distance = abs(new_x - center_x) + abs(new_y - center_y);
            if (distance < best_distance) {
                best_distance = distance;
                best_dir = dir;
            }
        }
    }
    
    return best_dir;
}

// Find the safest direction (highest mobility) when in danger
static int find_safe_direction(game *game_state, int player_id, int my_x, int my_y) {
    int best_dir = -1;
    int best_mobility = -1;
    
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            int degree, reachable;
            compute_mobility(game_state, player_id, new_x, new_y, &degree, &reachable);
            int total_mobility = degree + reachable;
            
            if (total_mobility > best_mobility) {
                best_mobility = total_mobility;
                best_dir = dir;
            }
        }
    }
    
    return best_dir;
}

// Check for potential oscillation by preferring moves away from recent positions
static int avoid_oscillation(game *game_state, int player_id, int my_x, int my_y, int *recent_positions, int pos_count) {
    // If we have recent position history, avoid going back
    if (pos_count > 0) {
        for (int dir = 0; dir < 8; dir++) {
            int new_x = my_x + dx[dir];
            int new_y = my_y + dy[dir];
            
            if (is_cell_free(game_state, player_id, new_x, new_y)) {
                // Check if this position was visited recently
                int was_recent = 0;
                for (int i = 0; i < pos_count && i < 3; i++) {
                    if (recent_positions[i*2] == new_x && recent_positions[i*2+1] == new_y) {
                        was_recent = 1;
                        break;
                    }
                }
                
                if (!was_recent) {
                    return dir; // Prefer moves to non-recent positions
                }
            }
        }
    }
    
    return -1; // No non-recent moves found
}

// Main move selection function with anti-stuck safeguards
static int choose_competitive_move(game *game_state, int player_id) {
    player *me = &game_state->players[player_id];
    int my_x = me->qx;
    int my_y = me->qy;
    
    // Count valid moves first
    int valid_moves = 0;
    int valid_directions[8];
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            valid_directions[valid_moves] = dir;
            valid_moves++;
        }
    }
    
    // If no valid moves, return -1 (will trigger random fallback)
    if (valid_moves == 0) {
        return -1;
    }
    
    // If only 1 valid move, take it immediately (avoid analysis paralysis)
    if (valid_moves == 1) {
        return valid_directions[0];
    }
    
    // If only 2-3 valid moves, prioritize safety over greed
    if (valid_moves <= 3) {
        // Check if we're in a dangerous position
        if (is_dangerous_position(game_state, player_id, my_x, my_y)) {
            int safe_dir = find_safe_direction(game_state, player_id, my_x, my_y);
            if (safe_dir != -1) {
                return safe_dir;
            }
        }
    }
    
    phase_weights weights = get_phase_weights(game_state, player_id);
    
    int best_direction = -1;
    double best_score = -1.0;
    
    // Evaluate all legal moves
    for (int dir = 0; dir < 8; dir++) {
        int new_x = my_x + dx[dir];
        int new_y = my_y + dy[dir];
        
        if (is_cell_free(game_state, player_id, new_x, new_y)) {
            double score = lookahead_score(game_state, player_id, new_x, new_y, LOOKAHEAD_DEPTH, &weights);
            
            // Tie-breaking: prefer higher immediate reward, then mobility, then density, then efficiency
            int immediate_reward = game_state->startBoard[new_y * game_state->width + new_x];
            int degree, reachable;
            compute_mobility(game_state, player_id, new_x, new_y, &degree, &reachable);
            double density = compute_density(game_state, player_id, new_x, new_y);
            double efficiency = compute_efficiency(game_state, player_id, new_x, new_y);
            
            // Apply tie-breaking as small adjustments to score - More greedy
            score += immediate_reward * 0.001;   // Much higher R(x) - 10x stronger
            score += degree * 0.00001;           // Larger deg(x)
            score += reachable * 0.000001;       // Larger reach_K(x)
            score += density * 0.0000001;        // Higher dens(x)
            score += efficiency * 0.00000001;    // Higher eff(x)
            
            // Safety bonus: prefer moves that don't lead to dangerous positions
            if (!is_dangerous_position(game_state, player_id, new_x, new_y)) {
                score += 0.1; // Small safety bonus
            }
            
            // Mobility bonus: prefer moves with higher immediate mobility
            score += degree * 0.01; // Small mobility bonus
            
            if (score > best_score) {
                best_score = score;
                best_direction = dir;
            }
        }
    }
    
    // If still no good move found, fall back to safest available move
    if (best_direction == -1) {
        best_direction = find_safe_direction(game_state, player_id, my_x, my_y);
    }
    
    return best_direction;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    (void)argv; // width/height passed for compatibility; read from shared memory
    
    // Open shared memory and semaphores
    game *game_state = open_shared_memory();
    semaphore_struct *sem_state = open_semaphore_memory();
    
    if (!game_state || !sem_state) {
        fprintf(stderr, "player_competitive: Failed to open shared memory or semaphores\n");
        return EXIT_FAILURE;
    }
    
    srand((unsigned int)(time(NULL) ^ getpid()));
    
    // Determine own player_id by PID (retry briefly)
    int player_id = -1;
    pid_t self = getpid();
    for (int attempt = 0; attempt < 200 && player_id < 0; attempt++) {
        if (acquire_read_access(sem_state) == 0) {
            for (unsigned int i = 0; i < game_state->cantPlayers; i++) {
                if (game_state->players[i].pid == self) {
                    player_id = (int)i;
                    break;
                }
            }
            release_read_access(sem_state);
        }
    }
    if (player_id < 0) {
        // Could not find myself in shared state
        close_semaphore_memory(sem_state);
        size_t sz = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
        close_shared_memory(game_state, sz);
        return EXIT_FAILURE;
    }
    
    int prev_count = -1;
    int recent_positions[6] = {-1, -1, -1, -1, -1, -1}; // Track last 3 positions (x,y pairs)
    int pos_history_count = 0;
    
    while (!game_state->ended) {
        // Read state: if blocked, close pipe and exit
        if (acquire_read_access(sem_state) == -1) {
            break;
        }
        int am_blocked = game_state->players[player_id].blocked;
        int count = (int)(game_state->players[player_id].validMove + game_state->players[player_id].invalidMove);
        int skip_write = (count == prev_count);
        prev_count = count;
        
        // Update position history
        int current_x = game_state->players[player_id].qx;
        int current_y = game_state->players[player_id].qy;
        
        // Shift history and add current position
        for (int i = 2; i >= 0; i--) {
            if (i > 0) {
                recent_positions[i*2] = recent_positions[(i-1)*2];
                recent_positions[i*2+1] = recent_positions[(i-1)*2+1];
            } else {
                recent_positions[0] = current_x;
                recent_positions[1] = current_y;
            }
        }
        if (pos_history_count < 3) pos_history_count++;
        
        int move_direction = choose_competitive_move(game_state, player_id);
        if (release_read_access(sem_state) == -1) {
            break;
        }
        if (am_blocked) {
            close(STDOUT_FILENO);
            break;
        }
        
        // Wait for turn granted by master
        if (wait_for_turn(sem_state, player_id) == -1) {
            break;
        }
        
        // If algorithm didn't find a direction, try fallback strategies
        if (move_direction == -1) {
            // First try: find any valid move (even if not optimal)
            for (int dir = 0; dir < 8; dir++) {
                int new_x = current_x + dx[dir];
                int new_y = current_y + dy[dir];
                if (is_cell_free(game_state, player_id, new_x, new_y)) {
                    move_direction = dir;
                    break;
                }
            }
            
            // Second try: if still no valid move, try to avoid recent positions
            if (move_direction == -1) {
                move_direction = avoid_oscillation(game_state, player_id, current_x, current_y, recent_positions, pos_history_count);
            }
            
            // Third try: emergency escape towards center
            if (move_direction == -1) {
                move_direction = emergency_escape(game_state, player_id, current_x, current_y);
            }
            
            // Last resort: random direction
            if (move_direction == -1) {
                move_direction = rand() % 8;
            }
        }
        
        if (!skip_write) {
            unsigned char b = (unsigned char)move_direction;
            ssize_t bytes_written = write(STDOUT_FILENO, &b, 1);
            if (bytes_written != 1) {
                perror("player_competitive write");
                break;
            }
        }
    }
    
    // Clean up
    close_semaphore_memory(sem_state);
    size_t game_size = sizeof(game) + (game_state->width * game_state->height * sizeof(int));
    close_shared_memory(game_state, game_size);
    
    return EXIT_SUCCESS;
}
