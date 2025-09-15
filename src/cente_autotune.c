#include "include/cente_autotune.h"
#include <math.h>

static float clamp01(float v){ return v<0.0f?0.0f:(v>1.0f?1.0f:v); }

float cente_autotune(const Board *b, int self_id, const InfluenceMap *inf,
                     cente_phase phase,
                     const cente_size_params *base,
                     cente_size_params *out) {
    *out = *base;
    int w = b->width, h = b->height; int n = w * h;
    // Density of occupation
    int occ = 0; for (int i = 0; i < n; i++) if (b->cells[i] <= 0) occ++;
    float density = (float)occ / (float)n; // [0,1]

    // Mobility
    int mob_count = 0;
    static const int dx[8] = {0,1,1,1,0,-1,-1,-1};
    static const int dy[8] = {-1,-1,0,1,1,1,0,-1};
    int x = b->head_x[self_id], y = b->head_y[self_id];
    for (int d = 0; d < 8; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx>=0 && nx<w && ny>=0 && ny<h) {
            int c = b->cells[ny*w+nx];
            int occu = c <= 0;
            for (int j=0;j<b->num_players && !occu;j++){
                if (j==self_id) continue;
                if (!b->blocked[j] && b->head_x[j]==(unsigned short)nx && b->head_y[j]==(unsigned short)ny) occu=1;
            }
            if (!occu) mob_count++;
        }
    }
    float mobility = (float)mob_count / 8.0f;

    // Tempo proxy near our head
    int i0 = y * w + x;
    float tempo = 0.5f * inf->t[i0] + 0.5f * (1.0f - inf->grad[i0]);
    tempo = clamp01(tempo);

    // Adjust sigma: sparser -> higher sigma; dense -> lower sigma
    float base_sigma = base->sigma;
    float sigma = base_sigma * (0.9f + 0.4f * (1.0f - density));
    if (phase == PHASE_ENDGAME) sigma *= 0.9f; // sharpen in endgame
    if (sigma < 1.0f) sigma = 1.0f;
    out->sigma = sigma;

    // Adjust c_puct: higher when mobility high and tempo high (explore), lower if low time or dense
    float cp = base->mcts.c_puct;
    cp *= (0.9f + 0.4f * tempo) * (0.9f + 0.2f * mobility);
    cp *= (0.95f + 0.1f * (1.0f - density));
    if (phase == PHASE_ENDGAME) cp *= 0.9f;
    out->mcts.c_puct = cp;

    // Adjust K (k_base): more in opening, fewer in dense endgames
    int k = base->mcts.k_base;
    float kf = (phase == PHASE_OPENING ? 1.2f : (phase == PHASE_MIDGAME ? 1.0f : 0.8f));
    kf *= (0.9f + 0.3f * mobility);
    int kadj = (int)lroundf((float)k * kf);
    if (kadj < 6) kadj = 6;
    if (kadj > CENTE_MAX_CHILDREN) kadj = CENTE_MAX_CHILDREN;
    out->mcts.k_base = kadj;

    // Adjust budget if tempo is high (critical): small burst
    int budget = base->budget_ms;
    if (tempo > 0.7f) budget = (int)(budget * 1.15f);
    out->budget_ms = budget;

    // Keep other thresholds proportional
    out->mcts.k_inc = base->mcts.k_inc;
    out->mcts.t1 = base->mcts.t1; out->mcts.t2 = base->mcts.t2; out->mcts.t3 = base->mcts.t3;
    out->mcts.rollout_depth = base->mcts.rollout_depth;
    out->mcts.epsilon_rollout = base->mcts.epsilon_rollout;

    return tempo;
}


