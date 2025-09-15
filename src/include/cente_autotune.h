#ifndef CENTE_AUTOTUNE_H
#define CENTE_AUTOTUNE_H

#include "cente_board.h"
#include "cente_influence.h"
#include "cente_eval.h"
#include "cente_config.h"

// Computes dynamic parameters based on board size, density, mobility and tempo.
// Inputs: base size params from cente_defaults_for_size, detected phase, influence map.
// Output: tuned params (out), and returns a tempo_proxy in [0,1].
float cente_autotune(const Board *b, int self_id, const InfluenceMap *inf,
                     cente_phase phase,
                     const cente_size_params *base,
                     cente_size_params *out);

#endif


