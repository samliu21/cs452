#include "rps_server.h"
#include "name_server.h"
#include "stringmap.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include <stdlib.h>

int64_t signup()
{
    int rps_server_tid = who_is(RPS_SERVER_NAME);
    char buf[2];
    buf[0] = SIGNUP;
    buf[1] = 0;
    int ret = send(rps_server_tid, buf, 2, NULL, 0);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int64_t play(rps_move_t move)
{
    int rps_server_tid = who_is(RPS_SERVER_NAME);
    char buf[3];
    buf[0] = PLAY;
    buf[1] = move;
    buf[2] = 0;
    char result;
    int ret = send(rps_server_tid, buf, 2, &result, 1);
    if (ret < 0) {
        return -1;
    }
    return result;
}

int64_t quit()
{
    int rps_server_tid = who_is(RPS_SERVER_NAME);
    char buf[2];
    buf[0] = QUIT;
    buf[1] = 0;
    int ret = send(rps_server_tid, buf, 2, NULL, 0);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

// finds game by tid.
int find_game(uint64_t tid, rps_game_t* games, uint64_t free_games)
{
    for (int i = 0; i < MAX_GAMES; ++i) {
        if (free_games & ((uint64_t)1 << i)) { // check if game is active
            if (games[i].tid_1 == tid || games[i].tid_2 == tid) {
                return i;
            }
        }
    }
    return -1;
}

// finds a free game.
int find_free_game(uint64_t free_games)
{
    for (int i = 0; i < MAX_GAMES; ++i) {
        if (!(free_games & ((uint64_t)1 << i))) {
            return i;
        }
    }
    return -1;
}

void k2_rps_server()
{
    rps_game_t games[MAX_GAMES];
    // encodes which games are free. '0' means free and '1' means active.
    uint64_t free_games = 0;
    uint64_t caller_tid;
    uint64_t waiting_tid = NO_PLAYER;
    char buf[3];
    register_as(RPS_SERVER_NAME);
    char* moves[3] = { "rock", "paper", "scissors" };

    for (;;) {

        int64_t sz = receive(&caller_tid, buf, 2);
        buf[sz] = 0;

        switch (buf[0]) {
        case SIGNUP: {
            ASSERT(buf[1] == 0, "'signup' takes no arguments");
            if (waiting_tid == NO_PLAYER) {
                waiting_tid = caller_tid;
            } else { // set up game and ready both clients.
                // find and claim a free game.
                int i = find_free_game(free_games);
                free_games |= (uint64_t)1 << i;
                ASSERT(i >= 0, "no free games left.");

                // initialize game.
                games[i].move_1 = PENDING;
                games[i].move_2 = PENDING;
                games[i].tid_1 = waiting_tid;
                games[i].tid_2 = caller_tid;

                // reply to clients.
                reply_null(waiting_tid);
                reply_null(caller_tid);

                waiting_tid = 0;
            }
            break;
        }
        case PLAY: {
            ASSERT(buf[2] == 0, "'play' takes 1 argument");
            ASSERT(buf[1] != PENDING, "valid moves are rock, paper, or scissors.");
            int i = find_game(caller_tid, games, free_games);
            ASSERTF(i >= 0, "task %d is not in a game.");
            rps_game_t* game = &(games[i]);

            // check if other player has quit
            if (game->tid_1 == NO_PLAYER || game->tid_2 == NO_PLAYER) {
                uart_printf(CONSOLE, "task %d wins by forfeit.", caller_tid, moves[(int) buf[1]]);
                reply(caller_tid, "F", 1);
            } else {
                // update caller's move
                uart_printf(CONSOLE, "task %d plays %s.", caller_tid, moves[(int) buf[1]]);
                if (game->tid_1 == caller_tid) {
                    game->move_1 = buf[1];
                } else {
                    game->move_2 = buf[1];
                }

                // get results of game
                if (game->move_1 != PENDING && game->move_2 != PENDING) {
                    // rock = 0, paper = 1, scissors = 2
                    int result = (game->move_1 - game->move_2 + 3) % 3;
                    switch (result) {
                    case 0: // tie
                        uart_printf(CONSOLE, "%s ties %s, tie game.", moves[game->move_1], moves[game->move_2]);
                        reply(game->tid_1, "T", 1);
                        reply(game->tid_2, "T", 1);
                        break;
                    case 1: // player 1 wins
                        uart_printf(CONSOLE, "%s beats %s, task %d wins.", moves[game->move_1], moves[game->move_2], game->tid_1);
                        reply(game->tid_1, "W", 1);
                        reply(game->tid_2, "L", 1);
                        break;
                    case 2: // player 2 wins
                        uart_printf(CONSOLE, "%s beats %s, task %d wins.", moves[game->move_2], moves[game->move_1], game->tid_2);
                        reply(game->tid_1, "L", 1);
                        reply(game->tid_2, "W", 1);
                        break;
                    }
                } else {
                    // game is not finished yet.
                    break;
                }
            }
            // game is over, reclaim game
            free_games &= ~((uint64_t)1 << i);
            break;
        }
        case QUIT: {
            ASSERT(buf[1] == 0, "'quit' takes no arguments");
            int i = find_game(caller_tid, games, free_games);
            ASSERTF(i >= 0, "task %d is not in a game.");
            rps_game_t* game = &(games[i]);

            // update game state
            uart_printf(CONSOLE, "task %d quits.", caller_tid);
            if (game->tid_1 == caller_tid) {
                game->tid_1 = NO_PLAYER;
            } else {
                game->tid_2 = NO_PLAYER;
            }

            // check if game over and reclaim game.
            if (game->tid_1 == NO_PLAYER && game->tid_2 == NO_PLAYER) {
                free_games &= ~((uint64_t)1 << i);
            }

            reply_null(caller_tid);
            break;
        }
        default: {
            ASSERT(0, "invalid command");
        }
        }
    }
}
