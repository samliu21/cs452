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

char play(rps_move_t move)
{
    int rps_server_tid = who_is(RPS_SERVER_NAME);
    char buf[3];
    buf[0] = PLAY;
    buf[1] = move;
    buf[2] = 0;
    char result;
    int ret = send(rps_server_tid, buf, 2, &result, 1);
    if (ret < 0) {
        return FAILED;
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
            uart_printf(CONSOLE, "task %d signs up.\r\n", caller_tid);

            if (waiting_tid == NO_PLAYER) {
                waiting_tid = caller_tid;
            } else { // set up game and ready both clients.
                // find and claim a free game.
                int game_id = find_free_game(free_games);
                free_games |= (uint64_t)1 << game_id;
                ASSERT(game_id >= 0, "no free games left.");

                // initialize game.
                games[game_id].move_1 = PENDING;
                games[game_id].move_2 = PENDING;
                games[game_id].tid_1 = waiting_tid;
                games[game_id].tid_2 = caller_tid;

                // reply to clients.
                reply_null(waiting_tid);
                reply_null(caller_tid);
                uart_printf(CONSOLE, "task %d and task %d are now playing.\r\n", waiting_tid, caller_tid);

                waiting_tid = 0;
            }
            break;
        }
        case PLAY: {
            ASSERT(buf[2] == 0, "'play' takes 1 argument");
            ASSERT(buf[1] != PENDING, "valid moves are rock, paper, or scissors.");

            int game_id = find_game(caller_tid, games, free_games);
            ASSERTF(game_id >= 0, "task %d is not in a game.", caller_tid);
            rps_game_t* game = &(games[game_id]);

            // check if other player has quit
            if (game->tid_1 == NO_PLAYER || game->tid_2 == NO_PLAYER) {
                uart_printf(CONSOLE, "task %d wins by forfeit.\r\n", caller_tid, moves[(int)buf[1]]);
                reply_uint(caller_tid, ABANDONED);

                // game is over, reclaim game
                free_games &= ~((uint64_t)1 << game_id);
                break;
            } else {
                // update caller's move
                uart_printf(CONSOLE, "task %d plays %s.\r\n", caller_tid, moves[(int)buf[1]]);
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
                        uart_printf(CONSOLE, "%s ties %s, tie game.\r\n", moves[game->move_1], moves[game->move_2]);
                        reply_uint(game->tid_1, TIE);
                        reply_uint(game->tid_2, TIE);
                        break;
                    case 1: // player 1 wins
                        uart_printf(CONSOLE, "%s beats %s, task %d wins.\r\n", moves[game->move_1], moves[game->move_2], game->tid_1);
                        reply_uint(game->tid_1, WIN);
                        reply_uint(game->tid_2, LOSE);
                        break;
                    case 2: // player 2 wins
                        uart_printf(CONSOLE, "%s beats %s, task %d wins.\r\n", moves[game->move_2], moves[game->move_1], game->tid_2);
                        reply_uint(game->tid_1, LOSE);
                        reply_uint(game->tid_2, WIN);
                        break;
                    }

                    // reset game state
                    game->move_1 = PENDING;
                    game->move_2 = PENDING;
                } else {
                    // game is not finished yet.
                    break;
                }
            }
            break;
        }
        case QUIT: {
            ASSERT(buf[1] == 0, "'quit' takes no arguments");

            int game_id = find_game(caller_tid, games, free_games);
            ASSERTF(game_id >= 0, "task %d is not in a game.", caller_tid);
            rps_game_t* game = &(games[game_id]);

            // update game state
            uart_printf(CONSOLE, "task %d quits.\r\n", caller_tid);
            uint64_t other_tid;
            rps_move_t other_move;
            if (game->tid_1 == caller_tid) {
                game->tid_1 = NO_PLAYER;
                other_tid = game->tid_2;
                other_move = game->move_2;
            } else {
                game->tid_2 = NO_PLAYER;
                other_tid = game->tid_1;
                other_move = game->move_1;
            }

            // check if game over and reclaim game.
            if (other_tid == NO_PLAYER) {
                free_games &= ~((uint64_t)1 << game_id);
            } else if (other_move != PENDING) {
                // inform other play of forfeit if they're waiting for a response
                reply_uint(other_tid, ABANDONED);
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
