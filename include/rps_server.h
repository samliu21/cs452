#ifndef _rps_server_h_
#define _rps_server_h_

#include "common.h"

#define SIGNUP 'S'
#define PLAY 'P'
#define QUIT 'Q'
#define RPS_SERVER_NAME "rps_server"
#define MAX_GAMES 8
#define NO_PLAYER 0

typedef enum {
    ROCK,
    PAPER,
    SCISSORS,
    PENDING,
} rps_move_t;

typedef struct rps_game_t {
    uint64_t tid_1;
    uint64_t tid_2;
    rps_move_t move_1;
    rps_move_t move_2;
} rps_game_t;

int64_t signup();
int64_t play(rps_move_t move);
int64_t quit();
void k2_rps_server();

#endif
