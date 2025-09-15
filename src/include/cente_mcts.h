#ifndef CENTE_MCTS_H
#define CENTE_MCTS_H

#include <stdint.h>
#include "cente_board.h"
#include "cente_influence.h"
#include "cente_eval.h"
#include "cente_config.h"

typedef struct {
    uint64_t hash;
    int visits;
    float value_sum;
    int num_children;
    CenteMove actions[CENTE_MAX_CHILDREN];
    float priors[CENTE_MAX_CHILDREN];
    int child_visits[CENTE_MAX_CHILDREN];
    float child_q[CENTE_MAX_CHILDREN];
} MCTSNode;

void mcts_reset(void);
void mcts_set_params(const cente_mcts_params *params);
void mcts_set_size_params(const cente_size_params *size_params);

CenteMove mcts_select(const Board *root, int self_id, const InfluenceMap *inf,
                      const cente_weights *w, const cente_mcts_params *params,
                      int budget_ms);

#endif


