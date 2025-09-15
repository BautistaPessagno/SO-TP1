#ifndef CENTE_EVAL_H
#define CENTE_EVAL_H

#include "cente_board.h"
#include "cente_influence.h"
#include "cente_config.h"

typedef enum { PHASE_OPENING = 0, PHASE_MIDGAME = 1, PHASE_ENDGAME = 2 } cente_phase;

void phase_detect(const Board *b, const InfluenceMap *inf, cente_phase *phase_out, cente_weights *w_out);

float centrality_score(const Board *b, CenteMove m);
float mobility_score(const Board *b, int player_id);
float stability_proxy(const Board *b, const InfluenceMap *inf, CenteMove m);
float risk_proxy(const Board *b, const InfluenceMap *inf, CenteMove m);
float tempo_score_1ply(const Board *b, int player_id, const InfluenceMap *inf, CenteMove m);

float prior_cente(const Board *b, int player_id, const InfluenceMap *inf, CenteMove m);
float value_eval(const Board *b, int player_id, const InfluenceMap *inf, const cente_weights *w);

#endif


