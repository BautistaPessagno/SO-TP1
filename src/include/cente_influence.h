#ifndef CENTE_INFLUENCE_H
#define CENTE_INFLUENCE_H

#include "cente_board.h"

typedef struct {
    float *iyou;   // size w*h
    float *irival; // size w*h
    float *t;      // territory potential = max(iyou - irival, 0)
    float *grad;   // gradient magnitude of T
    int width;
    int height;
} InfluenceMap;

InfluenceMap *influence_create(int width, int height);
void influence_destroy(InfluenceMap *map);
void compute_influence_full(const Board *b, int self_id, float sigma, InfluenceMap *out);

#endif


