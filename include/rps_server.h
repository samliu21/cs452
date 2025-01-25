#ifndef _rps_server_h_
#define _rps_server_h_

#include "common.h"

#define RPS_SERVER_NAME "rps_server"
#define MAX_GAMES 8
#define NO_PLAYER 0

typedef enum {
    SIGNUP = 'S',
    PLAY = 'P',
    QUIT = 'Q',
} rps_server_request_t;

typedef enum {
    ABANDONED = 'A',
    TIE = 'T',
    WIN = 'W',
    LOSE = 'L',
    FAILED = 'F',
} rps_server_response_t;

typedef enum {
    ROCK = 0,
    PAPER = 1,
    SCISSORS = 2,
    PENDING,
} rps_move_t;

typedef struct rps_player_t {
    uint64_t tid;
    rps_move_t move;
} rps_player_t;

typedef struct rps_game_t {
    rps_player_t p1;
    rps_player_t p2;
} rps_game_t;

int64_t signup();
rps_server_response_t play(rps_move_t move);
int64_t quit();
void k2_rps_server();

#endif
