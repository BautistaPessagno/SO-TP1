#ifndef CENTE_CONFIG_H
#define CENTE_CONFIG_H

#include <stddef.h>

#define CENTE_MAX_PLAYERS 9
#define CENTE_MAX_CHILDREN 16
#define CENTE_MAX_NODES 20000

#ifndef TT_SIZE_MB
#define TT_SIZE_MB 32
#endif

#ifndef KERNEL_SIGMA_FACTOR
#define KERNEL_SIGMA_FACTOR 6
#endif

typedef struct {
    float alpha; // Territorio
    float beta;  // Centralidad
    float gamma; // Movilidad
    float delta; // Estabilidad
    float eps;   // Tempo
    float zeta;  // Riesgo
} cente_weights;

typedef struct {
    float c_puct;
    int k_base;
    int k_inc;
    int t1;
    int t2;
    int t3;
    int rollout_depth;
    float epsilon_rollout;
} cente_mcts_params;

typedef struct {
    int budget_ms;
    float sigma;
    cente_mcts_params mcts;
} cente_size_params;

// Size tiers
typedef enum { CENTE_SMALL = 0, CENTE_MEDIUM = 1, CENTE_LARGE = 2 } cente_size_tier;

static inline cente_size_tier cente_select_size_tier(int width, int height) {
    int area = width * height;
    if (area <= 81) return CENTE_SMALL; // upto 9x9
    if (area <= 300) return CENTE_MEDIUM; // ~10x15
    return CENTE_LARGE; // 19x19+
}

static inline cente_size_params cente_defaults_for_size(int width, int height) {
    cente_size_tier tier = cente_select_size_tier(width, height);
    cente_size_params p;
    if (tier == CENTE_SMALL) {
        p.budget_ms = 25;
        p.sigma = (float)((width < height ? width : height) / (float)KERNEL_SIGMA_FACTOR);
        p.mcts.c_puct = 0.8f;
        p.mcts.k_base = 12;
        p.mcts.k_inc = 4;
        p.mcts.t1 = 32; p.mcts.t2 = 96; p.mcts.t3 = 192;
        p.mcts.rollout_depth = 6;
        p.mcts.epsilon_rollout = 0.15f;
    } else if (tier == CENTE_MEDIUM) {
        p.budget_ms = 80;
        p.sigma = (float)((width < height ? width : height) / (float)KERNEL_SIGMA_FACTOR);
        p.mcts.c_puct = 1.2f;
        p.mcts.k_base = 10;
        p.mcts.k_inc = 3;
        p.mcts.t1 = 48; p.mcts.t2 = 144; p.mcts.t3 = 384;
        p.mcts.rollout_depth = 8;
        p.mcts.epsilon_rollout = 0.20f;
    } else {
        p.budget_ms = 250;
        p.sigma = (float)((width < height ? width : height) / (float)KERNEL_SIGMA_FACTOR);
        p.mcts.c_puct = 1.6f;
        p.mcts.k_base = 8;
        p.mcts.k_inc = 2;
        p.mcts.t1 = 64; p.mcts.t2 = 192; p.mcts.t3 = 512;
        p.mcts.rollout_depth = 10;
        p.mcts.epsilon_rollout = 0.25f;
    }
    if (p.sigma < 1.0f) p.sigma = 1.0f;
    return p;
}

#endif


