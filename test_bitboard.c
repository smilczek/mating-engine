#include <assert.h>

#include "base.h"
#include "chess.h"
#include "bitboard.c"

#include <string.h>
#include <stdio.h>

// helper for debugging
static void printBoardState(BoardState *BS) {
    for (int Rank = BOARDSIZE - 1; Rank > -1; --Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            char Square = BS->Board[Rank][File];
            if (Square == '\0') {
                Square = '.';
            }
            printf("%c", Square);
        }
        printf("\n");
    }
}

bool test_shift() {
    bool Success = true;

    Bitboard Pos = BB_RANK_1 & BB_FILE_A;
    Success &= bb_shift(Pos, DIR_NORTH) ==
        (BB_RANK_2 & BB_FILE_A);
    Success &= bb_shift(Pos, DIR_SOUTH) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_EAST) ==
        (BB_RANK_1 & BB_FILE_B);
    Success &= bb_shift(Pos, DIR_WEST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_NORTHEAST) ==
        (BB_RANK_2 & BB_FILE_B);
    Success &= bb_shift(Pos, DIR_NORTHWEST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_SOUTHEAST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_SOUTHWEST) ==
        0ULL;

    Pos = BB_RANK_8 & BB_FILE_H;
    Success &= bb_shift(Pos, DIR_NORTH) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_SOUTH) ==
        (BB_RANK_7 & BB_FILE_H);
    Success &= bb_shift(Pos, DIR_EAST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_WEST) ==
        (BB_RANK_8 & BB_FILE_G);
    Success &= bb_shift(Pos, DIR_NORTHEAST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_NORTHWEST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_SOUTHEAST) ==
        0ULL;
    Success &= bb_shift(Pos, DIR_SOUTHWEST) ==
        (BB_RANK_7 & BB_FILE_G);

    return Success;
}

static bool test_CoordToBB() {
    bool Success = true;

    Success &= ch_CoordToBB((Coord){0, 0}) == 1LL;
    Success &= ch_CoordToBB((Coord){7, 7}) == (1LL << 63);
    Success &= ch_CoordToBB((Coord){0, 7}) == (1LL << 7);
    Success &= ch_CoordToBB((Coord){7, 0}) == (1LL << 56);
    Success &= ch_CoordToBB((Coord){4, 4}) == (1LL << 36);

    return Success;
}


int main() {
    bool Success = true;

    Success &= test_shift();
    assert(Success);

    return Success;
}
