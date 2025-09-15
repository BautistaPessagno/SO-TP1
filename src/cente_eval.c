#include "include/cente_eval.h"
#include <math.h>

static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

void phase_detect(const Board *b, const InfluenceMap *inf, cente_phase *phase_out, cente_weights *w_out) {
    (void)inf;
    // Density: ratio of occupied cells
    int bw = b->width, h = b->height; int n = bw * h;
    int occ = 0; for (int i = 0; i < n; i++) if (b->cells[i] <= 0) occ++;
    float density = (float)occ / (float)n;
    // Simple phase heuristic
    cente_phase ph = density < 0.25f ? PHASE_OPENING : (density < 0.70f ? PHASE_MIDGAME : PHASE_ENDGAME);
    *phase_out = ph;
    cente_weights w;
    if (ph == PHASE_OPENING) {
        w.alpha = 0.25f; w.beta = 0.35f; w.gamma = 0.20f; w.delta = 0.05f; w.eps = 0.20f; w.zeta = 0.10f;
    } else if (ph == PHASE_MIDGAME) {
        w.alpha = 0.30f; w.beta = 0.25f; w.gamma = 0.20f; w.delta = 0.10f; w.eps = 0.20f; w.zeta = 0.15f;
    } else {
        w.alpha = 0.30f; w.beta = 0.15f; w.gamma = 0.10f; w.delta = 0.25f; w.eps = 0.10f; w.zeta = 0.25f;
    }
    *w_out = w;
}

float centrality_score(const Board *b, CenteMove m) {
    float cx = (b->width - 1) * 0.5f;
    float cy = (b->height - 1) * 0.5f;
    float nx = (m.x - cx) / (cx + 1e-6f);
    float ny = (m.y - cy) / (cy + 1e-6f);
    float d = sqrtf(nx * nx + ny * ny);
    float c = 1.0f - d;
    if (c < 0.0f) c = 0.0f;
    if (c > 1.0f) c = 1.0f;
    return c;
}

float mobility_score(const Board *b, int player_id) {
    static const int dx[8] = {0,1,1,1,0,-1,-1,-1};
    static const int dy[8] = {-1,-1,0,1,1,1,0,-1};
    int x = b->head_x[player_id]; int y = b->head_y[player_id];
    int legal = 0;
    for (int d = 0; d < 8; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx >= 0 && nx < b->width && ny >= 0 && ny < b->height) {
            int c = b->cells[ny * b->width + nx];
            int occ = c <= 0;
            for (int j = 0; j < b->num_players && !occ; j++) {
                if (j == player_id) continue;
                if (!b->blocked[j] && b->head_x[j] == (unsigned short)nx && b->head_y[j] == (unsigned short)ny) occ = 1;
            }
            if (!occ) legal++;
        }
    }
    return clamp01((float)legal / 8.0f);
}

float stability_proxy(const Board *b, const InfluenceMap *inf, CenteMove m) {
    int i = m.y * b->width + m.x;
    float support = inf->iyou[i];
    float border = inf->grad[i];
    float s = 0.7f * support + 0.3f * (1.0f - border);
    return clamp01(s);
}

float risk_proxy(const Board *b, const InfluenceMap *inf, CenteMove m) {
    int i = m.y * b->width + m.x;
    float t = inf->t[i];
    float g = inf->grad[i];
    float risk = 0.6f * g + 0.4f * (t > 0.75f ? 1.0f : 0.0f);
    return clamp01(risk);
}

static float cell_value_norm(const Board *b, CenteMove m) {
    int v = b->cells[m.y * b->width + m.x];
    if (v <= 0) return 0.0f;
    // Assume typical max <= 255; clip otherwise
    float vn = (float)v / 255.0f;
    if (vn > 1.0f) vn = 1.0f;
    if (vn < 0.0f) vn = 0.0f;
    return vn;
}

static float claim_urgency(const Board *b, const InfluenceMap *inf, CenteMove m) {
    // High if cell has value and rival pressure/border high
    float val = cell_value_norm(b, m);
    int i = m.y * b->width + m.x;
    float rival = inf->irival[i];
    float grad = inf->grad[i];
    float you = inf->iyou[i];
    float pressure = clamp01(0.6f * rival + 0.4f * grad - 0.3f * you + 0.5f);
    return clamp01(val * pressure);
}

float tempo_score_1ply(const Board *b, int player_id, const InfluenceMap *inf, CenteMove m) {
    (void)player_id;
    // Approx: IM ~ our local value - opponent local best
    float our = 0.5f * centrality_score(b, m) + 0.5f * inf->t[m.y * b->width + m.x];
    // Opponent best in neighborhood of our destination
    static const int dx[8] = {0,1,1,1,0,-1,-1,-1};
    static const int dy[8] = {-1,-1,0,1,1,1,0,-1};
    float opp_best = 0.0f;
    for (int d = 0; d < 8; d++) {
        int nx = m.x + dx[d], ny = m.y + dy[d];
        if (nx < 0 || ny < 0 || nx >= b->width || ny >= b->height) continue;
        float v = 0.5f * (1.0f - centrality_score(b, (CenteMove){nx, ny})) + 0.5f * inf->t[ny * b->width + nx];
        if (v > opp_best) opp_best = v;
    }
    float im = our - opp_best;
    return clamp01(0.5f + 0.5f * im);
}

float prior_cente(const Board *b, int player_id, const InfluenceMap *inf, CenteMove m) {
    float c = centrality_score(b, m);
    float stab = stability_proxy(b, inf, m);
    float risk = risk_proxy(b, inf, m);
    float tempo = tempo_score_1ply(b, player_id, inf, m);
    float tval = inf->t[m.y * b->width + m.x];
    float claim = claim_urgency(b, inf, m);
    // Emphasize claiming valuable cells without abandoning cente principles
    float prior = 0.20f * c + 0.25f * tval + 0.15f * tempo + 0.10f * stab + 0.05f * (1.0f - risk) + 0.25f * claim;
    return clamp01(prior);
}

float value_eval(const Board *b, int player_id, const InfluenceMap *inf, const cente_weights *w) {
    // Territory around our head
    float t_head = inf->t[b->head_y[player_id] * b->width + b->head_x[player_id]];
    // Include current score ratio to value claiming points
    unsigned int sum_scores = 0; for (int p = 0; p < b->num_players; p++) sum_scores += b->score[p];
    float score_ratio = sum_scores ? (float)b->score[player_id] / (float)sum_scores : 0.0f;
    t_head = clamp01(0.7f * t_head + 0.3f * score_ratio);
    float c = centrality_score(b, (CenteMove){ b->head_x[player_id], b->head_y[player_id] });
    float mob = mobility_score(b, player_id);
    float stab = stability_proxy(b, inf, (CenteMove){ b->head_x[player_id], b->head_y[player_id] });
    float risk = risk_proxy(b, inf, (CenteMove){ b->head_x[player_id], b->head_y[player_id] });
    float tempo = 0.5f; // default neutral
    float V = w->alpha * t_head + w->beta * c + w->gamma * mob + w->delta * stab + w->eps * tempo - w->zeta * risk;
    // Clip to [0,1]
    return clamp01(V);
}


