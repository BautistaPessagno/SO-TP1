#ifndef CENTE_ZOBRIST_H
#define CENTE_ZOBRIST_H

#include <stdint.h>
#include "cente_board.h"

void zobrist_init(int width, int height, int num_players);
uint64_t zobrist_hash_board(const Board *b);

#endif


