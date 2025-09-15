// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "include/cente_mcts.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static cente_mcts_params G_PARAMS;

static unsigned long long now_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long long)tv.tv_sec * 1000ULL +
         (unsigned long long)(tv.tv_usec / 1000ULL);
}

void mcts_reset(void) {}

void mcts_set_params(const cente_mcts_params *params) { G_PARAMS = *params; }
void mcts_set_size_params(const cente_size_params *size_params) {
  G_PARAMS = size_params->mcts;
}

static void softmax_inplace(float *a, int n) {
  float mx = -1e30f;
  for (int i = 0; i < n; i++)
    if (a[i] > mx)
      mx = a[i];
  float sum = 0.0f;
  for (int i = 0; i < n; i++) {
    a[i] = expf(a[i] - mx);
    sum += a[i];
  }
  float inv = sum > 1e-9f ? 1.0f / sum : 1.0f;
  for (int i = 0; i < n; i++)
    a[i] *= inv;
}

static int generate_candidates(const Board *b, int self_id,
                               const InfluenceMap *inf, CenteMove out[],
                               float priors[], int maxk) {
  // All legal up to maxk, scored by prior_cente
  CenteMove tmp[64];
  int n = board_legal_moves(b, self_id, tmp, 64);
  if (n == 0)
    return 0;
  if (n > maxk)
    n = maxk;
  for (int i = 0; i < n; i++) {
    out[i] = tmp[i];
    priors[i] = prior_cente(b, self_id, inf, tmp[i]);
  }
  // Sort by prior descending to claim urgent cells first
  for (int i = 0; i < n; i++) {
    int best = i;
    for (int j = i + 1; j < n; j++)
      if (priors[j] > priors[best])
        best = j;
    if (best != i) {
      float tp = priors[i];
      priors[i] = priors[best];
      priors[best] = tp;
      CenteMove tm = out[i];
      out[i] = out[best];
      out[best] = tm;
    }
  }
  softmax_inplace(priors, n);
  return n;
}

static float rollout_policy(const Board *b, int self_id,
                            const InfluenceMap *inf, const cente_weights *w) {
  // One-step value proxy
  return value_eval(b, self_id, inf, w);
}

static float simulate_once(Board *root, int self_id, const InfluenceMap *inf,
                           const cente_weights *w) {
  // Stateless light simulation: evaluate current state
  (void)root;
  (void)self_id;
  return rollout_policy(root, self_id, inf, w);
}

CenteMove mcts_select(const Board *root, int self_id, const InfluenceMap *inf,
                      const cente_weights *w, const cente_mcts_params *params,
                      int budget_ms) {
  unsigned long long deadline = now_ms() + (unsigned long long)budget_ms;
  CenteMove best = {.x = root->head_x[self_id], .y = root->head_y[self_id]};

  // Root candidates
  CenteMove actions[CENTE_MAX_CHILDREN];
  float priors[CENTE_MAX_CHILDREN];
  int cap = params ? params->k_base : CENTE_MAX_CHILDREN;
  if (cap > CENTE_MAX_CHILDREN)
    cap = CENTE_MAX_CHILDREN;
  if (cap < 1)
    cap = 1;
  int K = generate_candidates(root, self_id, inf, actions, priors, cap);
  if (K == 0)
    return best;

  float q[CENTE_MAX_CHILDREN];
  int nvis[CENTE_MAX_CHILDREN];
  for (int i = 0; i < K; i++) {
    q[i] = 0.0f;
    nvis[i] = 0;
  }
  int total = 0;

  while ((int)(deadline - now_ms()) > 1) {
    // PUCT selection at root only (shallow tree due to time constraints)
    int arg = 0;
    float best_score = -1e30f;
    float sqrt_total = sqrtf((float)(total + 1));
    for (int i = 0; i < K; i++) {
      float c_puct = params ? params->c_puct : G_PARAMS.c_puct;
      float u =
          q[i] + c_puct * priors[i] * sqrt_total / (1.0f + (float)nvis[i]);
      if (u > best_score) {
        best_score = u;
        arg = i;
      }
    }
    // Apply action virtually
    Board bcopy = *root;
    size_t n = (size_t)root->width * (size_t)root->height;
    bcopy.cells = (int *)malloc(n * sizeof(int));
    if (!bcopy.cells)
      break;
    memcpy(bcopy.cells, root->cells, n * sizeof(int));
    board_apply_move(&bcopy, self_id, actions[arg]);

    float r = simulate_once(&bcopy, self_id, inf, w);

    free(bcopy.cells);
    total++;
    nvis[arg]++;
    q[arg] += (r - q[arg]) / (float)nvis[arg];
  }

  // Choose by visits or q
  int best_i = 0;
  float best_v = -1e30f;
  for (int i = 0; i < K; i++) {
    float v = (float)nvis[i] + 1e-3f * q[i];
    if (v > best_v) {
      best_v = v;
      best_i = i;
    }
  }
  return actions[best_i];
}
