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

static bool test_bbSquare() {
    bool Success = true;

    // Corner squares
    Success &= bb_Square(0) == 1ULL;
    Success &= bb_Square(63) == (1ULL << 63);

    // Edge squares
    Success &= bb_Square(7) == (1ULL << 7);
    Success &= bb_Square(56) == (1ULL << 56);

    // Interior square (e4 = 36)
    Success &= bb_Square(36) == (1ULL << 36);

    // Each result must have exactly one bit set
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard bb = bb_Square(sq);
        // Verify only the expected bit is set
        Success &= (bb ^ (1ULL << sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbPopcount() {
    bool Success = true;

    Success &= bb_popcount(0ULL) == 0;
    Success &= bb_popcount(1ULL) == 1;
    Success &= bb_popcount(BB_RANK_1) == 8;
    Success &= bb_popcount(BB_FILE_A) == 8;
    Success &= bb_popcount(BB_RANK_1 & BB_FILE_A) == 1;
    Success &= bb_popcount(0xFFFFFFFFFFFFFFFFULL) == 64;
    Success &= bb_popcount(0x5555555555555555ULL) == 32;

    return Success;
}

static bool test_bbLsbMsb() {
    bool Success = true;

    // Single square lsb
    Success &= bb_lsb(bb_Square(0)) == 0;
    Success &= bb_lsb(bb_Square(63)) == 63;
    Success &= bb_lsb(bb_Square(32)) == 32;

    // Rank/file lsb
    Success &= bb_lsb(BB_RANK_1) == 0;       // a1 is LSB of rank 1
    Success &= bb_lsb(BB_FILE_A) == 0;        // a1 is LSB of file A
    Success &= bb_lsb(BB_RANK_8) == 56;       // a8 is LSB of rank 8

    // Specific bit patterns
    Success &= bb_lsb(0xAAAAAAAAAAAAAAAAULL) == 1;  // all odd bits set -> LSB is bit 1
    Success &= bb_lsb(0x5555555555555555ULL) == 0;  // all even bits set -> LSB is bit 0

    // Multiple bits set: lsb returns lowest
    Bitboard multi = bb_Square(5) | bb_Square(20);
    Success &= bb_lsb(multi) == 5;

    // Single square msb
    Success &= bb_msb(bb_Square(0)) == 0;
    Success &= bb_msb(bb_Square(63)) == 63;
    Success &= bb_msb(bb_Square(32)) == 32;

    // Rank/file msb
    Success &= bb_msb(BB_RANK_1) == 7;        // h1 is MSB of rank 1
    Success &= bb_msb(BB_RANK_8) == 63;       // h8 is MSB of rank 8
    Success &= bb_msb(BB_FILE_A) == 56;       // a8 is MSB of file A

    // Multiple bits set: msb returns highest
    Success &= bb_msb(multi) == 20;

    // Round-trip: lsb(Square(x)) == x, msb(Square(x)) == x
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard bb = bb_Square(sq);
        Success &= bb_lsb(bb) == sq;
        Success &= bb_msb(bb) == sq;
    }

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
    Success &= test_bbSquare();
    Success &= test_bbPopcount();
    Success &= test_bbLsbMsb();
    Success &= test_CoordToBB();
    assert(Success);

    return !Success;
}
