#include "include/cente_zobrist.h"
#include <stdlib.h>

static uint64_t *cell_keys = NULL; // per cell two states: free vs occupied/head by pid sign collapsed
static uint64_t *head_keys = NULL; // per player per cell
static int g_w = 0, g_h = 0, g_p = 0;

static inline uint64_t rng64(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

void zobrist_init(int width, int height, int num_players) {
    if (cell_keys) { free(cell_keys); cell_keys = NULL; }
    if (head_keys) { free(head_keys); head_keys = NULL; }
    g_w = width; g_h = height; g_p = num_players;
    size_t n = (size_t)width * (size_t)height;
    cell_keys = (uint64_t *)malloc(n * sizeof(uint64_t));
    head_keys = (uint64_t *)malloc((size_t)num_players * n * sizeof(uint64_t));
    uint64_t seed = 88172645463393265ULL;
    for (size_t i = 0; i < n; i++) cell_keys[i] = rng64(&seed);
    for (size_t p = 0; p < (size_t)num_players; p++)
        for (size_t i = 0; i < n; i++) head_keys[p * n + i] = rng64(&seed);
}

uint64_t zobrist_hash_board(const Board *b) {
    size_t n = (size_t)b->width * (size_t)b->height;
    uint64_t h = 0;
    for (size_t i = 0; i < n; i++) {
        if (b->cells[i] <= 0) h ^= cell_keys[i];
    }
    for (int p = 0; p < b->num_players; p++) {
        size_t i = (size_t)b->head_y[p] * (size_t)b->width + (size_t)b->head_x[p];
        h ^= head_keys[p * n + i];
    }
    h ^= (uint64_t)b->current_player * 0x9e3779b97f4a7c15ULL;
    return h;
}


