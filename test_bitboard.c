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

static bool test_bbPopLsb() {
    bool Success = true;

    // Single square: pop returns 5, bitboard becomes 0
    {
        Bitboard b = bb_Square(5);
        int sq = bb_pop_lsb(&b);
        Success &= sq == 5;
        Success &= b == 0ULL;
    }

    // Two squares: first call returns 3, second returns 17, then 0
    {
        Bitboard b = bb_Square(3) | bb_Square(17);
        int s1 = bb_pop_lsb(&b);
        Success &= s1 == 3;
        int s2 = bb_pop_lsb(&b);
        Success &= s2 == 17;
        Success &= b == 0ULL;
    }

    // Full rank iteration: collect squares from BB_RANK_1, verify all 8 squares 0..7
    {
        Bitboard b = BB_RANK_1;
        int squares[8];
        int count = 0;
        while (b) {
            squares[count++] = bb_pop_lsb(&b);
        }
        Success &= count == 8;
        for (int i = 0; i < 8; ++i) {
            Success &= squares[i] == i;
        }
    }

    // File A iteration: verify ascending order 0,8,16,...,56
    {
        Bitboard b = BB_FILE_A;
        int expected[] = {0, 8, 16, 24, 32, 40, 48, 56};
        int idx = 0;
        while (b) {
            int s = bb_pop_lsb(&b);
            Success &= s == expected[idx];
            ++idx;
        }
        Success &= idx == 8;
    }

    // Popcount integration: iterate a random-ish bitboard, count iterations == original popcount
    {
        Bitboard b = 0xACEFACEACEACEACEULL;
        int origPop = bb_popcount(b);
        int iterCount = 0;
        while (b) {
            bb_pop_lsb(&b);
            ++iterCount;
        }
        Success &= iterCount == origPop;
        Success &= b == 0ULL;
    }

    return Success;
}

static bool test_bbMoreThanOne() {
    bool Success = true;

    // Zero bits: false
    Success &= !bb_moreThanOne(0ULL);

    // Single bit at square 0: false
    Success &= !bb_moreThanOne(bb_Square(0));

    // Single bit at square 63 (top): false
    Success &= !bb_moreThanOne(bb_Square(63));

    // Two bits set: true
    Success &= bb_moreThanOne(bb_Square(0) | bb_Square(7));

    // Eight bits set (entire rank 1): true
    Success &= bb_moreThanOne(BB_RANK_1);

    // All 64 bits: true
    Success &= bb_moreThanOne(0xFFFFFFFFFFFFFFFFULL);

    // Single high bit 0x8000000000000000: false
    Success &= !bb_moreThanOne(0x8000000000000000ULL);

    return Success;
}

static bool test_bbNextBit() {
    bool Success = true;

    // Empty bitboard: must return -1 immediately
    {
        Bitboard b = 0ULL;
        Success &= bb_nextBit(&b) == -1;
        Success &= b == 0ULL;
    }

    // Single square: returns the square, then -1
    {
        Bitboard b = bb_Square(23);
        Success &= bb_nextBit(&b) == 23;
        Success &= bb_nextBit(&b) == -1;
        Success &= b == 0ULL;
    }

    // Multi-square iteration: collect all squares from BB_RANK_1
    {
        Bitboard b = BB_RANK_1;
        int squares[8];
        int count = 0;
        while (true) {
            int sq = bb_nextBit(&b);
            if (sq == -1) break;
            squares[count++] = sq;
        }
        Success &= count == 8;
        for (int i = 0; i < 8; ++i) {
            Success &= squares[i] == i;
        }
    }

    // Full board iteration: iterate all 64 squares, verify count matches popcount
    {
        Bitboard b = 0xFFFFFFFFFFFFFFFFULL;
        int count = 0;
        while (true) {
            int sq = bb_nextBit(&b);
            if (sq == -1) break;
            Success &= sq >= 0 && sq < 64;
            ++count;
        }
        Success &= count == 64;
        Success &= b == 0ULL;
    }

    // Ascending order: results must come back LSB-first (increasing)
    {
        Bitboard b = BB_FILE_A;
        int prev = -1;
        while (true) {
            int sq = bb_nextBit(&b);
            if (sq == -1) break;
            Success &= sq > prev;
            prev = sq;
        }
    }

    // Arbitrary pattern: two scattered squares
    {
        Bitboard b = bb_Square(12) | bb_Square(45);
        int s1 = bb_nextBit(&b);
        Success &= s1 == 12;
        int s2 = bb_nextBit(&b);
        Success &= s2 == 45;
        int s3 = bb_nextBit(&b);
        Success &= s3 == -1;
        Success &= b == 0ULL;
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
    Success &= test_bbPopLsb();
    Success &= test_bbMoreThanOne();
    Success &= test_bbNextBit();
    Success &= test_CoordToBB();
    assert(Success);

    return !Success;
}
