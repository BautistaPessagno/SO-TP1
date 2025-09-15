#ifndef CENTE_BOARD_H
#define CENTE_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define CENTE_MAX_PLAYERS 9

typedef struct { int x, y; } CenteMove;

typedef struct {
    int width;
    int height;
    int num_players;
    int current_player; // index 0..num_players-1

    unsigned short head_x[CENTE_MAX_PLAYERS];
    unsigned short head_y[CENTE_MAX_PLAYERS];
    unsigned char blocked[CENTE_MAX_PLAYERS];
    unsigned int score[CENTE_MAX_PLAYERS];

    // Grid encoding: >0 free points; <=0 occupied by body or head (-pid)
    int *cells; // size width*height
} Board;

Board *board_create_from_shared(int width, int height,
                                int num_players,
                                const unsigned short *head_x,
                                const unsigned short *head_y,
                                const unsigned char *blocked,
                                const unsigned int *score,
                                const int *cells,
                                int self_player);

void board_destroy(Board *b);

bool board_is_inside(const Board *b, int x, int y);
bool board_is_legal(const Board *b, int player_id, CenteMove m);
int  board_legal_moves(const Board *b, int player_id, CenteMove out[], int max_out);

void board_apply_move(Board *b, int player_id, CenteMove m);
uint64_t board_hash(const Board *b);

#endif


