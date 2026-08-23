#include <assert.h>

#include "base.h"
#include "chess.h"
#include "bitboard.c"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

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


static bool test_EnPassant_initNone() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;  // Explicitly initialize to "no en passant"

    // Must be -1 after explicit init
    Success &= BS.EnPassant == -1;

    return Success;
}

static bool test_EnPassant_setGet() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;

    // d5 = rank 5 (index 4) * 8 + file d (index 3) = 35
    BS.EnPassant = 35;
    Success &= BS.EnPassant == 35;

    // Change to f3 = rank 3 (index 2) * 8 + file f (index 5) = 21
    BS.EnPassant = 21;
    Success &= BS.EnPassant == 21;

    // Reset to none
    BS.EnPassant = -1;
    Success &= BS.EnPassant == -1;

    return Success;
}

static bool test_EnPassant_validSquares() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;

    // Valid en passant squares are on ranks 3 and 6 (indices 16..23 and 40..47)
    // Rank 3 (white's en passant zone): a3=16, b3=17, ..., h3=23
    BS.EnPassant = 16;  // a3
    Success &= BS.EnPassant >= 0 && BS.EnPassant < 64;

    BS.EnPassant = 23;  // h3
    Success &= BS.EnPassant >= 0 && BS.EnPassant < 64;

    // Rank 6 (black's en passant zone): a6=40, b6=41, ..., h6=47
    BS.EnPassant = 40;  // a6
    Success &= BS.EnPassant >= 0 && BS.EnPassant < 64;

    BS.EnPassant = 47;  // h6
    Success &= BS.EnPassant >= 0 && BS.EnPassant < 64;

    return Success;
}

static bool test_EnPassant_noneMeansNoRight() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;

    // -1 means no en passant target — this is the sentinel
    Success &= BS.EnPassant == -1;

    // A value of 0 would mean a1 IS the en passant square (unlikely but valid as int)
    // Only -1 should represent "none"
    BS.EnPassant = 0;
    Success &= BS.EnPassant != -1;  // Square 0 is a valid square, not "none"

    BS.EnPassant = 63;
    Success &= BS.EnPassant != -1;  // Square 63 is also valid

    return Success;
}

static bool test_BitboardState_zeroInit() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // All piece bitboards must be zero
    for (int c = 0; c < 2; ++c) {
        for (int pt = 0; pt < 6; ++pt) {
            Success &= BS.Pieces[c][pt] == 0ULL;
        }
    }
    Success &= BS.Occupancy[WHITE] == 0ULL;
    Success &= BS.Occupancy[BLACK] == 0ULL;
    Success &= BS.AllPieces == 0ULL;
    Success &= BS.Blocked == 0ULL;

    // Note: zero-init gives EnPassant==0 which is technically a valid square.
    // Proper initialization should set EnPassant=-1 explicitly.
    // This test verifies the raw zero-init behavior; see test_EnPassant_initNone for correct usage.

    return Success;
}

static bool test_BitboardState_setPiece() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Place white king on e1 (square 4)
    BS.Pieces[WHITE][KING] = bb_Square(4);

    // Verify the king bitboard is set correctly
    Success &= BS.Pieces[WHITE][KING] == bb_Square(4);

    // Verify other piece bitboards are still zero
    for (int pt = 0; pt < 6; ++pt) {
        if (pt == KING) continue;
        Success &= BS.Pieces[WHITE][pt] == 0ULL;
    }
    for (int pt = 0; pt < 6; ++pt) {
        Success &= BS.Pieces[1][pt] == 0ULL;
    }

    return Success;
}

static bool test_BitboardState_updateOccupancy() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Place several pieces: white pawns on rank 2, white king on e1
    BS.Pieces[0][0] = BB_RANK_2;  // 8 pawns
    BS.Pieces[0][4] = bb_Square(4); // king on e1

    bb_updateOccupancy(&BS);

    // Occupancy[WHITE] must equal union of all white piece bitboards
    Bitboard whiteExpected = BB_RANK_2 | bb_Square(4);
    Success &= BS.Occupancy[WHITE] == whiteExpected;

    // Occupancy[BLACK] must be zero (no black pieces placed)
    Success &= BS.Occupancy[BLACK] == 0ULL;

    // AllPieces == Occupancy[WHITE] | Occupancy[BLACK]
    Success &= BS.AllPieces == (BS.Occupancy[WHITE] | BS.Occupancy[BLACK]);
    Success &= BS.AllPieces == whiteExpected;

    // Blocked == AllPieces
    Success &= BS.Blocked == BS.AllPieces;

    // Popcount of allPieces should be 9
    Success &= bb_popcount(BS.AllPieces) == 9;

    return Success;
}

static bool test_BitboardState_fullPosition() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // White: pawns rank 2, rooks a1/h1, knights b1/g1, bishops c1/f1, queen d1, king e1
    BS.Pieces[WHITE][PAWN]   = BB_RANK_2;
    BS.Pieces[WHITE][KNIGHT]  = bb_Square(1) | bb_Square(6);
    BS.Pieces[WHITE][BISHOP]  = bb_Square(2) | bb_Square(5);
    BS.Pieces[WHITE][ROOK]    = bb_Square(0) | bb_Square(7);
    BS.Pieces[WHITE][QUEEN]    = bb_Square(3);
    BS.Pieces[WHITE][KING]     = bb_Square(4);

    // Black: pawns rank 7, rooks a8/h8, knights b8/g8, bishops c8/f8, queen d8, king e8
    BS.Pieces[BLACK][PAWN]   = BB_RANK_7;
    BS.Pieces[BLACK][KNIGHT]  = bb_Square(57) | bb_Square(62);
    BS.Pieces[BLACK][BISHOP]  = bb_Square(58) | bb_Square(61);
    BS.Pieces[BLACK][ROOK]    = bb_Square(56) | bb_Square(63);
    BS.Pieces[BLACK][QUEEN]    = bb_Square(59);
    BS.Pieces[BLACK][KING]     = bb_Square(60);

    bb_updateOccupancy(&BS);

    // White occupancy: rank 1 + rank 2
    Bitboard whiteExpected = BB_RANK_1 | BB_RANK_2;
    Success &= BS.Occupancy[WHITE] == whiteExpected;

    // Black occupancy: rank 7 + rank 8
    Bitboard blackExpected = BB_RANK_7 | BB_RANK_8;
    Success &= BS.Occupancy[BLACK] == blackExpected;

    // AllPieces == Occupancy[WHITE] | Occupancy[BLACK]
    Bitboard totalExpected = whiteExpected | blackExpected;
    Success &= BS.AllPieces == totalExpected;
    Success &= BS.AllPieces == (BS.Occupancy[WHITE] | BS.Occupancy[BLACK]);

    // Blocked == AllPieces
    Success &= BS.Blocked == BS.AllPieces;

    // Should have 32 pieces total
    Success &= bb_popcount(BS.AllPieces) == 32;

    return Success;
}

static bool test_Occupancy_twoPieces() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Place white king on e1 (square 4) and black queen on d8 (square 59)
    BS.Pieces[WHITE][KING] = bb_Square(4);
    BS.Pieces[BLACK][QUEEN] = bb_Square(59);

    bb_updateOccupancy(&BS);

    // Occupancy[WHITE] has only e1
    Success &= BS.Occupancy[WHITE] == bb_Square(4);

    // Occupancy[BLACK] has only d8
    Success &= BS.Occupancy[BLACK] == bb_Square(59);

    // AllPieces has both e1 and d8
    Success &= BS.AllPieces == (bb_Square(4) | bb_Square(59));

    // Blocked == AllPieces
    Success &= BS.Blocked == BS.AllPieces;

    // AllPieces == Occupancy[WHITE] | Occupancy[BLACK]
    Success &= BS.AllPieces == (BS.Occupancy[WHITE] | BS.Occupancy[BLACK]);

    return Success;
}

static bool test_Castling_zeroInit() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Zero-init state has Castling == 0 (no rights)
    Success &= BS.Castling == 0;
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_setWK() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Set WK right
    bb_setCastleRight(&BS, BB_CASTLE_WK);

    Success &= BS.Castling == BB_CASTLE_WK;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_allRights() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Set all 4 rights (value 15)
    bb_setCastleRight(&BS, BB_CASTLE_WK);
    bb_setCastleRight(&BS, BB_CASTLE_WQ);
    bb_setCastleRight(&BS, BB_CASTLE_BK);
    bb_setCastleRight(&BS, BB_CASTLE_BQ);

    Success &= BS.Castling == 15;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_clearBK() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Set all 4 rights
    BS.Castling = 15;

    // Clear BK right — remaining should be 11 (0b1011 = WK+WQ+BQ)
    bb_clearCastleRight(&BS, BB_CASTLE_BK);

    Success &= BS.Castling == 11;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_roundTrip() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Set WQ + BK (bits 1 and 2 => value 6)
    bb_setCastleRight(&BS, BB_CASTLE_WQ);
    bb_setCastleRight(&BS, BB_CASTLE_BK);

    Success &= BS.Castling == 6;
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    // Clear WQ — only BK should remain (value 4)
    bb_clearCastleRight(&BS, BB_CASTLE_WQ);

    Success &= BS.Castling == 4;
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_clearUnset() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));

    // Only set WK
    bb_setCastleRight(&BS, BB_CASTLE_WK);

    // Try clearing BQ which isn't set — no crash, value unchanged
    int beforeClear = BS.Castling;
    bb_clearCastleRight(&BS, BB_CASTLE_BQ);

    Success &= BS.Castling == beforeClear;
    Success &= BS.Castling == BB_CASTLE_WK;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    return Success;
}

static bool test_Castling_constants() {
    bool Success = true;

    // Verify constant values match Stockfish convention
    Success &= BB_CASTLE_WK == 1;   // bit 0
    Success &= BB_CASTLE_WQ == 2;   // bit 1
    Success &= BB_CASTLE_BK == 4;   // bit 2
    Success &= BB_CASTLE_BQ == 8;   // bit 3

    return Success;
}

// --- bb_parseFEN() tests ---

static bool test_bbParseFEN_startPosition() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bb_parseFEN(&BS, fen);

    // White pawns on rank 2
    Success &= BS.Pieces[WHITE][PAWN] == BB_RANK_2;

    // Black pawns on rank 7
    Success &= BS.Pieces[BLACK][PAWN] == BB_RANK_7;

    // White knights b1(1), g1(6)
    Success &= BS.Pieces[WHITE][KNIGHT] == (bb_Square(1) | bb_Square(6));

    // White bishops c1(2), f1(5)
    Success &= BS.Pieces[WHITE][BISHOP] == (bb_Square(2) | bb_Square(5));

    // White rooks a1(0), h1(7)
    Success &= BS.Pieces[WHITE][ROOK] == (bb_Square(0) | bb_Square(7));

    // White queen d1(3)
    Success &= BS.Pieces[WHITE][QUEEN] == bb_Square(3);

    // White king e1(4)
    Success &= BS.Pieces[WHITE][KING] == bb_Square(4);

    // Black knights b8(57), g8(62)
    Success &= BS.Pieces[BLACK][KNIGHT] == (bb_Square(57) | bb_Square(62));

    // Black bishops c8(58), f8(61)
    Success &= BS.Pieces[BLACK][BISHOP] == (bb_Square(58) | bb_Square(61));

    // Black rooks a8(56), h8(63)
    Success &= BS.Pieces[BLACK][ROOK] == (bb_Square(56) | bb_Square(63));

    // Black queen d8(59)
    Success &= BS.Pieces[BLACK][QUEEN] == bb_Square(59);

    // Black king e8(60)
    Success &= BS.Pieces[BLACK][KING] == bb_Square(60);

    // ActiveColor should be WHITE
    Success &= BS.ActiveColor == WHITE;

    // Castling: all four rights => 15
    Success &= BS.Castling == 15;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    // EnPassant should be -1 (none)
    Success &= BS.EnPassant == -1;

    // Occupancy after parse: AllPieces should have 32 bits set
    Success &= bb_popcount(BS.AllPieces) == 32;

    // Occupancy[WHITE] should be rank 1 + rank 2
    Success &= BS.Occupancy[WHITE] == (BB_RANK_1 | BB_RANK_2);

    // Occupancy[BLACK] should be rank 7 + rank 8
    Success &= BS.Occupancy[BLACK] == (BB_RANK_7 | BB_RANK_8);

    return Success;
}

static bool test_bbParseFEN_enPassant() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/Pp6/8/PPPPPPPP/RNBQKBNR b KQkq a3 0 1";
    bb_parseFEN(&BS, fen);

    // EnPassant square a3 = file 0, rank 2 => 0*8 + 0 = 16
    Success &= BS.EnPassant == 16;

    // ActiveColor should be BLACK
    Success &= BS.ActiveColor == BLACK;

    return Success;
}

static bool test_bbParseFEN_reducedCastling() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kq - 0 1";
    bb_parseFEN(&BS, fen);

    // Castling: K(1) + q(8) = 9
    Success &= BS.Castling == 9;
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_WK);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_WQ);
    Success &= !bb_hasCastleRight(&BS, BB_CASTLE_BK);
    Success &= bb_hasCastleRight(&BS, BB_CASTLE_BQ);

    // EnPassant should be -1
    Success &= BS.EnPassant == -1;

    return Success;
}

static bool test_bbParseFEN_noCastling() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
    bb_parseFEN(&BS, fen);

    // No castling rights
    Success &= BS.Castling == 0;

    // EnPassant should be -1
    Success &= BS.EnPassant == -1;

    return Success;
}

static bool test_bbParseFEN_activeColorBlack() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";
    bb_parseFEN(&BS, fen);

    Success &= BS.ActiveColor == BLACK;

    return Success;
}

static bool test_bbParseFEN_halfmoveFullmove() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 50 10";
    bb_parseFEN(&BS, fen);

    Success &= BS.HalfmoveClock == 50;
    Success &= BS.FullmoveNumber == 10;

    return Success;
}

static bool test_bbParseFEN_kingsIndication() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqk1nr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bb_parseFEN(&BS, fen);

    // Black king is on e8(60), not f8(61)
    Success &= BS.Pieces[BLACK][KING] == bb_Square(60);

    // Black rook on h8(63), no rook on a8 because knight was moved
    // Actually FEN says "rnbqk1nr" so rook is on a8 and h8
    Success &= BS.Pieces[BLACK][ROOK] == (bb_Square(56) | bb_Square(63));

    return Success;
}

static bool test_bbParseFEN_midgame() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "r1bqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 1 2";
    bb_parseFEN(&BS, fen);

    // White pawns on rank 2
    Success &= BS.Pieces[WHITE][PAWN] == BB_RANK_2;

    // Black pawns on rank 7
    Success &= BS.Pieces[BLACK][PAWN] == BB_RANK_7;

    // Black knight on f6 (rank 5, file 5 => 5*8+5=45)
    Success &= BS.Pieces[BLACK][KNIGHT] == bb_Square(45);

    // White: R(a1), N(b1), B(c1), Q(d1), K(e1), B(f1), skip 1, R(h1)
    Success &= BS.Pieces[WHITE][KNIGHT] == bb_Square(1);
    Success &= BS.Pieces[WHITE][BISHOP] == (bb_Square(2) | bb_Square(5));
    Success &= BS.Pieces[WHITE][ROOK] == (bb_Square(0) | bb_Square(7));
    Success &= BS.Pieces[WHITE][QUEEN] == bb_Square(3);
    Success &= BS.Pieces[WHITE][KING] == bb_Square(4);

    // Black: r(a8), skip 1, b(c8), q(d8), k(e8), b(f8), skip 1, r(h8)
    Success &= BS.Pieces[BLACK][ROOK] == (bb_Square(56) | bb_Square(63));
    Success &= BS.Pieces[BLACK][BISHOP] == (bb_Square(58) | bb_Square(61));
    Success &= BS.Pieces[BLACK][QUEEN] == bb_Square(59);
    Success &= BS.Pieces[BLACK][KING] == bb_Square(60);

    // Active color white
    Success &= BS.ActiveColor == WHITE;

    // Halfmove clock = 1, fullmove = 2
    Success &= BS.HalfmoveClock == 1;
    Success &= BS.FullmoveNumber == 2;

    // Total: white(8p+1n+2b+2r+1q+1k=15) + black(8p+1n+2b+2r+1q+1k=15) = 30
    Success &= bb_popcount(BS.AllPieces) == 30;

    return Success;
}

// --- bb_fenToString() tests ---

static bool test_bbFenToString_startPosition() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_enPassant() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/Pp6/8/PPPPPPPP/RNBQKBNR b KQkq a3 0 1";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL ep: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_reducedCastling() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kq - 0 1";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL castling: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_emptyBoard() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;
    BS.ActiveColor = WHITE;
    BS.Castling = 0;
    BS.HalfmoveClock = 0;
    BS.FullmoveNumber = 1;

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    const char *expected = "8/8/8/8/8/8/8/8 w - - 0 1";
    Success &= strcmp(buf, expected) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL empty: got [%s]\nexpected[%s]\n", buf, expected);
    }

    return Success;
}

static bool test_bbFenToString_blackToMove() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL black: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_halfmoveFullmove() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 50 10";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL clocks: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_midgame() {
    bool Success = true;

    BitboardState BS;
    const char *fen = "r1bqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 1 2";
    bb_parseFEN(&BS, fen);

    char buf[256];
    bb_fenToString(&BS, buf, sizeof(buf));

    Success &= strcmp(buf, fen) == 0;
    if (!Success) {
        fprintf(stderr, "FAIL midgame: got [%s]\nexpected[%s]\n", buf, fen);
    }

    return Success;
}

static bool test_bbFenToString_roundtrip() {
    bool Success = true;

    // Several FEN positions to round-trip through parse -> serialize -> parse
    const char *fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "8/8/8/8/8/8/8/8 w - - 0 1",
        "8/8/8/8/8/8/8/8 b - - 0 1",
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w Qkq - 0 1",
        "rn1qkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    };
    int n = sizeof(fens) / sizeof(fens[0]);

    for (int i = 0; i < n; i++) {
        BitboardState BS1, BS2;
        char buf[256];

        bb_parseFEN(&BS1, fens[i]);
        bb_fenToString(&BS1, buf, sizeof(buf));
        bb_parseFEN(&BS2, buf);

        // Compare all bitboards
        for (int c = 0; c < 2; c++) {
            for (int pt = 0; pt < 6; pt++) {
                Success &= BS1.Pieces[c][pt] == BS2.Pieces[c][pt];
            }
        }
        for (int c = 0; c < 2; c++) {
            Success &= BS1.Occupancy[c] == BS2.Occupancy[c];
        }
        Success &= BS1.AllPieces == BS2.AllPieces;
        Success &= BS1.Blocked == BS2.Blocked;
        Success &= BS1.EnPassant == BS2.EnPassant;
        Success &= BS1.Castling == BS2.Castling;
        Success &= BS1.ActiveColor == BS2.ActiveColor;
        Success &= BS1.HalfmoveClock == BS2.HalfmoveClock;
        Success &= BS1.FullmoveNumber == BS2.FullmoveNumber;

        if (!Success) {
            fprintf(stderr, "FAIL roundtrip #%d: [%s] -> [%s]\n", i, fens[i], buf);
        }
    }

    return Success;
}

// --- bb_printBoard() tests ---

static bool test_bbPrintBoard_startPosition() {
    bool Success = true;

    BitboardState BS;
    bb_parseFEN(&BS, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Redirect stdout to temp file to capture output
    FILE *tmpf = tmpfile();
    assert(tmpf != NULL);
    FILE *oldStdout = stdout;
    dup2(fileno(tmpf), fileno(stdout));
    fflush(stdout);

    bb_printBoard(&BS);

    fflush(stdout);
    dup2(fileno(oldStdout), fileno(stdout));

    rewind(tmpf);
    char buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, tmpf);
    buf[n] = '\0';
    fclose(tmpf);

    // Output must contain rank labels '8' through '1'
    for (int r = 1; r <= 8; r++) {
        char label[4] = {0};
        snprintf(label, sizeof(label), "%d ", r);
        if (!strstr(buf, label)) {
            fprintf(stderr, "FAIL: missing rank label '%s'\n", label);
            Success = false;
        }
    }

    // Top row (rank 8) must show lowercase black pieces: r n b q k b n r
    if (!strstr(buf, "r | n | b | q | k | b | n | r")) {
        fprintf(stderr, "FAIL: top row should have lowercase black pieces\n");
        Success = false;
    }

    // Bottom row (rank 1) must show uppercase white pieces: R N B Q K B N R
    if (!strstr(buf, "R | N | B | Q | K | B | N | R")) {
        fprintf(stderr, "FAIL: bottom row should have uppercase white pieces\n");
        Success = false;
    }

    // Must contain individual piece characters
    const char *expectedPieces = "rnbqkpRNBQKP";
    for (int i = 0; expectedPieces[i]; i++) {
        if (!strchr(buf, expectedPieces[i])) {
            fprintf(stderr, "FAIL: missing piece char '%c'\n", expectedPieces[i]);
            Success = false;
        }
    }

    return Success;
}

static bool test_bbPrintBoard_emptyBoard() {
    bool Success = true;

    BitboardState BS;
    memset(&BS, 0, sizeof(BS));
    BS.EnPassant = -1;
    BS.ActiveColor = WHITE;

    FILE *tmpf = tmpfile();
    assert(tmpf != NULL);
    FILE *oldStdout = stdout;
    dup2(fileno(tmpf), fileno(stdout));
    fflush(stdout);

    bb_printBoard(&BS);

    fflush(stdout);
    dup2(fileno(oldStdout), fileno(stdout));

    rewind(tmpf);
    char buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, tmpf);
    buf[n] = '\0';
    fclose(tmpf);

    // Every square on an empty board should be '.'
    // Count dots: there should be 64 dots (one per square)
    int dotCount = 0;
    for (char *p = buf; *p; p++) {
        if (*p == '.') dotCount++;
    }
    if (dotCount < 64) {
        fprintf(stderr, "FAIL: empty board should have 64 dots, got %d\n", dotCount);
        Success = false;
    }

    return Success;
}

static bool test_bbPrintBoard_enPassantIndicator() {
    bool Success = true;

    BitboardState BS;
    bb_parseFEN(&BS, "rnbqkbnr/pppppppp/8/8/Pp6/8/PPPPPPPP/RNBQKBNR b KQkq a3 0 1");

    FILE *tmpf = tmpfile();
    assert(tmpf != NULL);
    FILE *oldStdout = stdout;
    dup2(fileno(tmpf), fileno(stdout));
    fflush(stdout);

    bb_printBoard(&BS);

    fflush(stdout);
    dup2(fileno(oldStdout), fileno(stdout));

    rewind(tmpf);
    char buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, tmpf);
    buf[n] = '\0';
    fclose(tmpf);

    // En passant square a3 should be marked with '+'
    // a3 is file 'a', rank 3. Look for '+' near rank 3 line.
    if (!strstr(buf, "+")) {
        fprintf(stderr, "FAIL: en passant square should be indicated with '+', output:\n%s\n", buf);
        Success = false;
    }

    return Success;
}

static bool test_bbPrintBoard_activeColor() {
    bool Success = true;

    BitboardState BS;
    bb_parseFEN(&BS, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    FILE *tmpf = tmpfile();
    assert(tmpf != NULL);
    FILE *oldStdout = stdout;
    dup2(fileno(tmpf), fileno(stdout));
    fflush(stdout);

    bb_printBoard(&BS);

    fflush(stdout);
    dup2(fileno(oldStdout), fileno(stdout));

    rewind(tmpf);
    char buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, tmpf);
    buf[n] = '\0';
    fclose(tmpf);

    // Output should indicate it's black's turn somehow (e.g., "b" or "Black")
    if (!strstr(buf, "b") && !strstr(buf, "Black")) {
        fprintf(stderr, "FAIL: active color indicator for black not found\n");
        Success = false;
    }

    return Success;
}

static bool test_bbPseudoAttacksKnight_corners() {
    bool Success = true;

    // a1 (sq 0): knight attacks c2(10) and b3(17) — exactly 2 attacks
    Bitboard a1_attacks = BB_PseudoAttacks_Knight[0];
    Success &= bb_popcount(a1_attacks) == 2;
    Success &= (a1_attacks & bb_Square(10)) != 0ULL;  // c2
    Success &= (a1_attacks & bb_Square(17)) != 0ULL;  // b3

    // h8 (sq 63): knight attacks f7(53) and g6(46) — exactly 2 attacks
    Bitboard h8_attacks = BB_PseudoAttacks_Knight[63];
    Success &= bb_popcount(h8_attacks) == 2;
    Success &= (h8_attacks & bb_Square(53)) != 0ULL;  // f7
    Success &= (h8_attacks & bb_Square(46)) != 0ULL;  // g6

    return Success;
}

static bool test_bbPseudoAttacksKnight_center() {
    bool Success = true;

    // d4 (sq 27): center square should have 8 attacks
    Bitboard d4_attacks = BB_PseudoAttacks_Knight[27];
    Success &= bb_popcount(d4_attacks) == 8;

    // Verify each of the 8 target squares
    Success &= (d4_attacks & bb_Square(17)) != 0ULL;  // b3
    Success &= (d4_attacks & bb_Square(33)) != 0ULL;  // b5
    Success &= (d4_attacks & bb_Square(10)) != 0ULL;  // c2
    Success &= (d4_attacks & bb_Square(42)) != 0ULL;  // c6
    Success &= (d4_attacks & bb_Square(12)) != 0ULL;  // e2
    Success &= (d4_attacks & bb_Square(44)) != 0ULL;  // e6
    Success &= (d4_attacks & bb_Square(21)) != 0ULL;  // f3
    Success &= (d4_attacks & bb_Square(37)) != 0ULL;  // f5

    return Success;
}

static bool test_bbPseudoAttacksKnight_onBoard() {
    bool Success = true;

    // For every square, all set bits in the attack mask must be < 64 (on-board)
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Knight[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_lsb(tmp);
            Success &= t >= 0 && t < 64;
            bb_pop_lsb(&tmp);
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksKnight_symmetry() {
    bool Success = true;

    // For every pair of squares (sq, t): if t is attacked by sq, then sq must be attacked by t
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Knight[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_pop_lsb(&tmp);
            // Symmetry: BB_PseudoAttacks_Knight[t] must have bit sq set
            Success &= (BB_PseudoAttacks_Knight[t] & bb_Square(sq)) != 0ULL;
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksKnight_expectedPopcounts() {
    bool Success = true;

    // Expected popcounts per square type:
    // Corner: 2 attacks (a1,h1,a8,h8)
    Success &= bb_popcount(BB_PseudoAttacks_Knight[0]) == 2;   // a1
    Success &= bb_popcount(BB_PseudoAttacks_Knight[7]) == 2;   // h1
    Success &= bb_popcount(BB_PseudoAttacks_Knight[56]) == 2;   // a8
    Success &= bb_popcount(BB_PseudoAttacks_Knight[63]) == 2;   // h8

    // Near-corner edge: 3 or 4 attacks
    Success &= bb_popcount(BB_PseudoAttacks_Knight[1]) == 3;   // b1
    Success &= bb_popcount(BB_PseudoAttacks_Knight[6]) == 3;   // g1
    Success &= bb_popcount(BB_PseudoAttacks_Knight[57]) == 3;   // b8
    Success &= bb_popcount(BB_PseudoAttacks_Knight[62]) == 3;   // g8

    // Center: 8 attacks (d4,e4,d5,e5)
    Success &= bb_popcount(BB_PseudoAttacks_Knight[27]) == 8;   // d4
    Success &= bb_popcount(BB_PseudoAttacks_Knight[35]) == 8;   // e4
    Success &= bb_popcount(BB_PseudoAttacks_Knight[34]) == 8;   // d5
    Success &= bb_popcount(BB_PseudoAttacks_Knight[42]) == 8;   // e5

    return Success;
}


static bool test_bbPseudoAttacksKing_corners() {
    bool Success = true;

    // a1 (sq 0): attacks b1(1), a2(8), b2(9) — exactly 3 attacks
    Bitboard a1_attacks = BB_PseudoAttacks_King[0];
    Success &= bb_popcount(a1_attacks) == 3;
    Success &= (a1_attacks & bb_Square(1)) != 0ULL;  // b1
    Success &= (a1_attacks & bb_Square(8)) != 0ULL;  // a2
    Success &= (a1_attacks & bb_Square(9)) != 0ULL;  // b2

    // h1 (sq 7): attacks g1(6), g2(14), h2(15) — exactly 3 attacks
    Bitboard h1_attacks = BB_PseudoAttacks_King[7];
    Success &= bb_popcount(h1_attacks) == 3;
    Success &= (h1_attacks & bb_Square(6)) != 0ULL;  // g1
    Success &= (h1_attacks & bb_Square(14)) != 0ULL; // g2
    Success &= (h1_attacks & bb_Square(15)) != 0ULL; // h2

    // a8 (sq 56): attacks b8(57), a7(48), b7(49) — exactly 3 attacks
    Bitboard a8_attacks = BB_PseudoAttacks_King[56];
    Success &= bb_popcount(a8_attacks) == 3;
    Success &= (a8_attacks & bb_Square(57)) != 0ULL; // b8
    Success &= (a8_attacks & bb_Square(48)) != 0ULL; // a7
    Success &= (a8_attacks & bb_Square(49)) != 0ULL; // b7

    // h8 (sq 63): attacks g8(62), h7(55), g7(54) — exactly 3 attacks
    Bitboard h8_attacks = BB_PseudoAttacks_King[63];
    Success &= bb_popcount(h8_attacks) == 3;
    Success &= (h8_attacks & bb_Square(62)) != 0ULL; // g8
    Success &= (h8_attacks & bb_Square(55)) != 0ULL; // h7
    Success &= (h8_attacks & bb_Square(54)) != 0ULL; // g7

    return Success;
}

static bool test_bbPseudoAttacksKing_center() {
    bool Success = true;

    // d4 (sq 27): center square should have 8 attacks
    Bitboard d4_attacks = BB_PseudoAttacks_King[27];
    Success &= bb_popcount(d4_attacks) == 8;

    // Verify each of the 8 target squares
    Success &= (d4_attacks & bb_Square(18)) != 0ULL;  // b3
    Success &= (d4_attacks & bb_Square(19)) != 0ULL;  // c3
    Success &= (d4_attacks & bb_Square(20)) != 0ULL;  // d3
    Success &= (d4_attacks & bb_Square(26)) != 0ULL;  // c4
    Success &= (d4_attacks & bb_Square(28)) != 0ULL;  // e4
    Success &= (d4_attacks & bb_Square(34)) != 0ULL;  // c5
    Success &= (d4_attacks & bb_Square(35)) != 0ULL;  // d5
    Success &= (d4_attacks & bb_Square(36)) != 0ULL;  // e5

    return Success;
}

static bool test_bbPseudoAttacksKing_edgeSquares() {
    bool Success = true;

    // Edge non-corner squares have 5 attacks
    // a-file edge (not corner): a2(8) has 5 neighbors
    Success &= bb_popcount(BB_PseudoAttacks_King[8]) == 5;   // a2
    Success &= bb_popcount(BB_PseudoAttacks_King[16]) == 5;  // a3
    Success &= bb_popcount(BB_PseudoAttacks_King[48]) == 5;   // a7

    // h-file edge (not corner): h2(15) has 5 neighbors
    Success &= bb_popcount(BB_PseudoAttacks_King[15]) == 5;   // h2
    Success &= bb_popcount(BB_PseudoAttacks_King[23]) == 5;   // h3
    Success &= bb_popcount(BB_PseudoAttacks_King[55]) == 5;    // h7

    // Rank 1 edge (not corner): b1(1) has 5 neighbors
    Success &= bb_popcount(BB_PseudoAttacks_King[1]) == 5;   // b1
    Success &= bb_popcount(BB_PseudoAttacks_King[2]) == 5;   // c1
    Success &= bb_popcount(BB_PseudoAttacks_King[5]) == 5;    // f1
    Success &= bb_popcount(BB_PseudoAttacks_King[6]) == 5;     // g1

    // Rank 8 edge (not corner): b8(57) has 5 neighbors
    Success &= bb_popcount(BB_PseudoAttacks_King[57]) == 5;   // b8
    Success &= bb_popcount(BB_PseudoAttacks_King[58]) == 5;   // c8
    Success &= bb_popcount(BB_PseudoAttacks_King[61]) == 5;    // f8
    Success &= bb_popcount(BB_PseudoAttacks_King[62]) == 5;     // g8

    return Success;
}

static bool test_bbPseudoAttacksKing_nearCorner() {
    bool Success = true;

    // Near-corner squares like b2(9) have 8 neighbors (all 8 surrounding squares exist)
    Success &= bb_popcount(BB_PseudoAttacks_King[9]) == 8;    // b2
    Success &= bb_popcount(BB_PseudoAttacks_King[14]) == 8;   // g2
    Success &= bb_popcount(BB_PseudoAttacks_King[57]) == 5;    // b8 — edge, not near-corner interior
    Success &= bb_popcount(BB_PseudoAttacks_King[62]) == 5;     // g8 — edge, not near-corner interior
    Success &= bb_popcount(BB_PseudoAttacks_King[49]) == 8;     // b7
    Success &= bb_popcount(BB_PseudoAttacks_King[54]) == 8;     // g7

    return Success;
}

static bool test_bbPseudoAttacksKing_onBoard() {
    bool Success = true;

    // For every square, all set bits must be on-board (< 64)
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_King[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_lsb(tmp);
            Success &= t >= 0 && t < 64;
            bb_pop_lsb(&tmp);
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksKing_symmetry() {
    bool Success = true;

    // For every pair (sq, t): if t is attacked by sq, then sq must be attacked by t
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_King[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_pop_lsb(&tmp);
            Success &= (BB_PseudoAttacks_King[t] & bb_Square(sq)) != 0ULL;
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksKing_noSelf() {
    bool Success = true;

    // No square should attack itself
    for (int sq = 0; sq < 64; ++sq) {
        Success &= (BB_PseudoAttacks_King[sq] & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbPseudoAttacksKing_expectedPopcounts() {
    bool Success = true;

    // Corners: 3 attacks each
    Success &= bb_popcount(BB_PseudoAttacks_King[0]) == 3;   // a1
    Success &= bb_popcount(BB_PseudoAttacks_King[7]) == 3;   // h1
    Success &= bb_popcount(BB_PseudoAttacks_King[56]) == 3;   // a8
    Success &= bb_popcount(BB_PseudoAttacks_King[63]) == 3;   // h8

    // Edges (non-corner): 5 attacks each
    Success &= bb_popcount(BB_PseudoAttacks_King[1]) == 5;   // b1
    Success &= bb_popcount(BB_PseudoAttacks_King[6]) == 5;   // g1
    Success &= bb_popcount(BB_PseudoAttacks_King[8]) == 5;   // a2
    Success &= bb_popcount(BB_PseudoAttacks_King[15]) == 5;  // h2
    Success &= bb_popcount(BB_PseudoAttacks_King[48]) == 5;   // a7
    Success &= bb_popcount(BB_PseudoAttacks_King[55]) == 5;   // h7
    Success &= bb_popcount(BB_PseudoAttacks_King[57]) == 5;   // b8
    Success &= bb_popcount(BB_PseudoAttacks_King[62]) == 5;   // g8

    // Near-corners (b2, b7, g2, g7): 8 attacks each
    Success &= bb_popcount(BB_PseudoAttacks_King[9]) == 8;    // b2
    Success &= bb_popcount(BB_PseudoAttacks_King[14]) == 8;   // g2
    Success &= bb_popcount(BB_PseudoAttacks_King[49]) == 8;    // b7
    Success &= bb_popcount(BB_PseudoAttacks_King[54]) == 8;    // g7

    // Inner squares (not on edge): 8 attacks each
    Success &= bb_popcount(BB_PseudoAttacks_King[11]) == 8;   // d3
    Success &= bb_popcount(BB_PseudoAttacks_King[27]) == 8;   // d4
    Success &= bb_popcount(BB_PseudoAttacks_King[35]) == 8;   // e4
    Success &= bb_popcount(BB_PseudoAttacks_King[36]) == 8;    // e5

    return Success;
}

static bool test_bbPawnAttacks_whiteCenter() {
    bool Success = true;

    // White pawn at e2 (sq=12): attacks f3(21) and d3(19) — exactly 2 bits
    Bitboard attacks = BB_PawnAttacks[WHITE][12];
    Success &= bb_popcount(attacks) == 2;
    Success &= (attacks & bb_Square(21)) != 0ULL;  // f3
    Success &= (attacks & bb_Square(19)) != 0ULL;  // d3

    return Success;
}

static bool test_bbPawnAttacks_whiteEdgeH() {
    bool Success = true;

    // White pawn at h2 (sq=15): attacks only g3(22) — 1 bit (edge)
    Bitboard attacks = BB_PawnAttacks[WHITE][15];
    Success &= bb_popcount(attacks) == 1;
    Success &= (attacks & bb_Square(22)) != 0ULL;  // g3

    return Success;
}

static bool test_bbPawnAttacks_whiteEdgeA() {
    bool Success = true;

    // White pawn at a2 (sq=8): attacks only b3(17) — 1 bit (edge)
    Bitboard attacks = BB_PawnAttacks[WHITE][8];
    Success &= bb_popcount(attacks) == 1;
    Success &= (attacks & bb_Square(17)) != 0ULL;  // b3

    return Success;
}

static bool test_bbPawnAttacks_blackCenter() {
    bool Success = true;

    // Black pawn at e7 (sq=52): attacks f6(45) and d6(43) — exactly 2 bits
    Bitboard attacks = BB_PawnAttacks[BLACK][52];
    Success &= bb_popcount(attacks) == 2;
    Success &= (attacks & bb_Square(45)) != 0ULL;  // f6
    Success &= (attacks & bb_Square(43)) != 0ULL;  // d6

    return Success;
}

static bool test_bbPawnAttacks_blackEdgeH() {
    bool Success = true;

    // Black pawn at h7 (sq=55): attacks only g6(46) — 1 bit
    Bitboard attacks = BB_PawnAttacks[BLACK][55];
    Success &= bb_popcount(attacks) == 1;
    Success &= (attacks & bb_Square(46)) != 0ULL;  // g6

    return Success;
}

static bool test_bbPawnAttacks_blackEdgeA() {
    bool Success = true;

    // Black pawn at a7 (sq=48): attacks only b6(41) — 1 bit
    Bitboard attacks = BB_PawnAttacks[BLACK][48];
    Success &= bb_popcount(attacks) == 1;
    Success &= (attacks & bb_Square(41)) != 0ULL;  // b6

    return Success;
}

static bool test_bbPawnAttacks_symmetry() {
    bool Success = true;

    // White pawn at rank N attacks rank N+1 diagonals
    // Black pawn at rank M attacks rank M-1 diagonals
    // Mirror: white at sq and black at (63-sq) should produce mirrored attack sets

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard wAtk = BB_PawnAttacks[WHITE][sq];
        Bitboard bAtk = BB_PawnAttacks[BLACK][63 - sq];
        // Mirroring: flip all bits of wAtk (reverse bit order) should equal bAtk
        Bitboard wMirrored = 0;
        for (int i = 0; i < 64; ++i) {
            if ((wAtk >> i) & 1) {
                wMirrored |= ((Bitboard)1 << (63 - i));
            }
        }
        Success &= wMirrored == bAtk;
    }

    return Success;
}

static bool test_bbPawnAttacks_noOffBoardBits() {
    bool Success = true;

    // For every square and both colors, all set bits must be on-board (< 64)
    for (int color = 0; color < 2; ++color) {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard attacks = BB_PawnAttacks[color][sq];
            Bitboard tmp = attacks;
            while (tmp) {
                int t = bb_lsb(tmp);
                Success &= t >= 0 && t < 64;
                bb_pop_lsb(&tmp);
            }
        }
    }

    return Success;
}

static bool test_bbPawnAttacks_whiteRank1() {
    bool Success = true;

    // White pawns on rank 1 (bottom row): attacks go to rank 2
    // c1 (sq=2): attacks b2(9) and d2(11)
    Bitboard attacks = BB_PawnAttacks[WHITE][2];
    Success &= bb_popcount(attacks) == 2;
    Success &= (attacks & bb_Square(9)) != 0ULL;   // b2
    Success &= (attacks & bb_Square(11)) != 0ULL;  // d2

    return Success;
}

static bool test_bbPawnAttacks_whiteRank8_none() {
    bool Success = true;

    // White pawn on rank 8: no forward squares exist, so no attacks
    for (int file = 0; file < 8; ++file) {
        int sq = 8 * 7 + file;  // rank 8 (index 7)
        Success &= BB_PawnAttacks[WHITE][sq] == 0ULL;
    }

    return Success;
}

static bool test_bbPawnAttacks_blackRank8_none() {
    bool Success = true;

    // Black pawn on rank 1 (index 0): no backward squares exist, so no attacks
    for (int file = 0; file < 8; ++file) {
        Success &= BB_PawnAttacks[BLACK][file] == 0ULL;
    }

    return Success;
}

static bool test_bbPawnAttacks_allPopcounts() {
    bool Success = true;

    // Verify expected popcount patterns for all 64 squares, both colors
    // Interior squares (not on a/h file): 2 attacks
    // Edge squares (a/h file, not on rank 1/8 for white, not rank 1/8 for black): 1 attack
    // Off-board direction: 0 attacks

    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;

        // White pawn attacks
        int wExpected = 0;
        if (rank < 7) {  // Can attack forward
            if (file > 0) wExpected++;  // NW
            if (file < 7) wExpected++;  // NE
        }
        Success &= bb_popcount(BB_PawnAttacks[WHITE][sq]) == wExpected;

        // Black pawn attacks
        int bExpected = 0;
        if (rank > 0) {  // Can attack backward
            if (file > 0) bExpected++;  // SW
            if (file < 7) bExpected++;  // SE
        }
        Success &= bb_popcount(BB_PawnAttacks[BLACK][sq]) == bExpected;
    }

    return Success;
}

static bool test_bbPawnPushes_whiteE2_doublePush() {
    bool Success = true;

    // White pawn at e2 (sq=14): pushes to e3(22) and e4(30) — 2 bits (double push from rank 1)
    Bitboard pushes = BB_PawnPushes[WHITE][14];
    Success &= bb_popcount(pushes) == 2;
    Success &= (pushes & bb_Square(22)) != 0ULL;  // e3
    Success &= (pushes & bb_Square(30)) != 0ULL;  // e4

    return Success;
}

static bool test_bbPawnPushes_whiteE3_singlePush() {
    bool Success = true;

    // White pawn at e3 (sq=22): pushes only to e4(30) — 1 bit (no double push)
    Bitboard pushes = BB_PawnPushes[WHITE][22];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(30)) != 0ULL;  // e4

    return Success;
}

static bool test_bbPawnPushes_whiteE7_promoRank() {
    bool Success = true;

    // White pawn at e7 (sq=46): pushes only to e8(54) — 1 bit (promotion rank)
    Bitboard pushes = BB_PawnPushes[WHITE][46];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(54)) != 0ULL;  // e8

    return Success;
}

static bool test_bbPawnPushes_whiteA1_rank0() {
    bool Success = true;

    // White pawn at a1 (sq=0): on rank 0, can only single push to a2(8). No double push (not on starting rank 1).
    Bitboard pushes = BB_PawnPushes[WHITE][0];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(8)) != 0ULL;   // a2

    return Success;
}

static bool test_bbPawnPushes_whiteRank8_none() {
    bool Success = true;

    // White pawn on rank 8 (top): no forward squares, no pushes
    for (int file = 0; file < 8; ++file) {
        int sq = 7 * 8 + file;
        Success &= BB_PawnPushes[WHITE][sq] == 0ULL;
    }

    return Success;
}

static bool test_bbPawnPushes_blackE7_doublePush() {
    bool Success = true;

    // Black pawn at e7 (sq=50): pushes to e6(42) and e5(34) — 2 bits
    Bitboard pushes = BB_PawnPushes[BLACK][50];
    Success &= bb_popcount(pushes) == 2;
    Success &= (pushes & bb_Square(42)) != 0ULL;  // e6
    Success &= (pushes & bb_Square(34)) != 0ULL;  // e5

    return Success;
}

static bool test_bbPawnPushes_blackE6_singlePush() {
    bool Success = true;

    // Black pawn at e6 (sq=42): pushes only to e5(34) — 1 bit
    Bitboard pushes = BB_PawnPushes[BLACK][42];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(34)) != 0ULL;  // e5

    return Success;
}

static bool test_bbPawnPushes_blackA8_rank0() {
    bool Success = true;

    // Black pawn at a8 (sq=56): on rank 7, can only single push to a7(48). No double push (not on starting rank 6).
    Bitboard pushes = BB_PawnPushes[BLACK][56];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(48)) != 0ULL;  // a7

    return Success;
}

static bool test_bbPawnPushes_hFile_white() {
    bool Success = true;

    // White pawn at h2 (sq=15): pushes to h3(23) and h4(31) — 2 bits
    Bitboard pushes = BB_PawnPushes[WHITE][15];
    Success &= bb_popcount(pushes) == 2;
    Success &= (pushes & bb_Square(23)) != 0ULL;  // h3
    Success &= (pushes & bb_Square(31)) != 0ULL;  // h4

    // White pawn at h3 (sq=23): pushes only to h4(31) — 1 bit
    pushes = BB_PawnPushes[WHITE][23];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(31)) != 0ULL;  // h4

    return Success;
}

static bool test_bbPawnPushes_hFile_black() {
    bool Success = true;

    // Black pawn at h7 (sq=55): pushes to h6(47) and h5(39) — 2 bits
    Bitboard pushes = BB_PawnPushes[BLACK][55];
    Success &= bb_popcount(pushes) == 2;
    Success &= (pushes & bb_Square(47)) != 0ULL;  // h6
    Success &= (pushes & bb_Square(39)) != 0ULL;  // h5

    // Black pawn at h6 (sq=47): pushes only to h5(39) — 1 bit
    pushes = BB_PawnPushes[BLACK][47];
    Success &= bb_popcount(pushes) == 1;
    Success &= (pushes & bb_Square(39)) != 0ULL;  // h5

    return Success;
}

static bool test_bbPawnPushes_noOffBoardBits() {
    bool Success = true;

    // For every square and both colors, all set bits must be on-board (< 64)
    for (int color = 0; color < 2; ++color) {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard pushes = BB_PawnPushes[color][sq];
            Bitboard tmp = pushes;
            while (tmp) {
                int t = bb_lsb(tmp);
                Success &= t >= 0 && t < 64;
                bb_pop_lsb(&tmp);
            }
        }
    }

    return Success;
}

static bool test_bbPawnPushes_allPopcounts() {
    bool Success = true;

    // Verify expected popcount for all 64 squares, both colors
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;

        // White pawn pushes
        int wExpected = 0;
        if (rank < 7) wExpected++;           // single push
        if (rank == 1) wExpected++;            // double push from rank 1
        Success &= bb_popcount(BB_PawnPushes[WHITE][sq]) == wExpected;

        // Black pawn pushes
        int bExpected = 0;
        if (rank > 0) bExpected++;             // single push
        if (rank == 6) bExpected++;             // double push from rank 6
        Success &= bb_popcount(BB_PawnPushes[BLACK][sq]) == bExpected;
    }

    return Success;
}

static bool test_bbPawnPushes_symmetry() {
    bool Success = true;

    // Mirror: white pushes at sq and black pushes at (63-sq) should be mirrored
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard wPushes = BB_PawnPushes[WHITE][sq];
        Bitboard bPushes = BB_PawnPushes[BLACK][63 - sq];
        Bitboard wMirrored = 0;
        for (int i = 0; i < 64; ++i) {
            if ((wPushes >> i) & 1) {
                wMirrored |= ((Bitboard)1 << (63 - i));
            }
        }
        Success &= wMirrored == bPushes;
    }

    return Success;
}

static bool test_bbPawnPushes_sameFile() {
    bool Success = true;

    // Pushes must stay on the same file as the pawn
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        Bitboard fileMask = 0ULL;
        for (int r = 0; r < 8; ++r) {
            fileMask |= ((Bitboard)1 << (r * 8 + file));
        }
        for (int color = 0; color < 2; ++color) {
            Bitboard pushes = BB_PawnPushes[color][sq];
            Success &= (pushes & ~fileMask) == 0ULL;  // no bits outside the pawn's file
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksBishop_d4() {
    bool Success = true;

    // Bishop at d4 (sq=27): main diag (a1-h8) + anti-diag (h1-a8)
    // Main diag: c3(18), b2(9), a1(0), e5(36), f6(45), g7(54), h8(63) = 7
    // Anti-diag: g1(6), f2(13), e3(20), c5(34), b6(41), a7(48) = 6
    // Total = 13 unique squares
    Bitboard attacks = BB_PseudoAttacks_Bishop[27];
    Success &= bb_popcount(attacks) == 13;

    // Verify main diagonal squares
    Success &= (attacks & bb_Square(18)) != 0ULL;  // c3
    Success &= (attacks & bb_Square(9))  != 0ULL;  // b2
    Success &= (attacks & bb_Square(0))  != 0ULL;  // a1
    Success &= (attacks & bb_Square(36)) != 0ULL;  // e5
    Success &= (attacks & bb_Square(45)) != 0ULL;  // f6
    Success &= (attacks & bb_Square(54)) != 0ULL;  // g7
    Success &= (attacks & bb_Square(63)) != 0ULL;  // h8

    // Verify anti-diagonal squares
    Success &= (attacks & bb_Square(6))  != 0ULL;  // g1
    Success &= (attacks & bb_Square(13)) != 0ULL;  // f2
    Success &= (attacks & bb_Square(20)) != 0ULL;  // e3
    Success &= (attacks & bb_Square(34)) != 0ULL;  // c5
    Success &= (attacks & bb_Square(41)) != 0ULL;  // b6
    Success &= (attacks & bb_Square(48)) != 0ULL;  // a7

    // d4 itself must NOT be in the attack mask
    Success &= (attacks & bb_Square(27)) == 0ULL;

    return Success;
}

static bool test_bbPseudoAttacksBishop_a1() {
    bool Success = true;

    // Bishop at a1 (sq=0): only main diagonal a1-h8
    // b2(9), c3(18), d4(27), e5(36), f6(45), g7(54), h8(63) = 7 squares
    Bitboard attacks = BB_PseudoAttacks_Bishop[0];
    Success &= bb_popcount(attacks) == 7;
    Success &= (attacks & bb_Square(9))  != 0ULL;  // b2
    Success &= (attacks & bb_Square(18)) != 0ULL;  // c3
    Success &= (attacks & bb_Square(27)) != 0ULL;  // d4
    Success &= (attacks & bb_Square(36)) != 0ULL;  // e5
    Success &= (attacks & bb_Square(45)) != 0ULL;  // f6
    Success &= (attacks & bb_Square(54)) != 0ULL;  // g7
    Success &= (attacks & bb_Square(63)) != 0ULL;  // h8

    return Success;
}

static bool test_bbPseudoAttacksBishop_h1() {
    bool Success = true;

    // Bishop at h1 (sq=7): only anti-diagonal h1-a8
    // g2(14), f3(21), e4(28), d5(35), c6(42), b7(49), a8(56) = 7 squares
    Bitboard attacks = BB_PseudoAttacks_Bishop[7];
    Success &= bb_popcount(attacks) == 7;
    Success &= (attacks & bb_Square(14)) != 0ULL;  // g2
    Success &= (attacks & bb_Square(21)) != 0ULL;  // f3
    Success &= (attacks & bb_Square(28)) != 0ULL;  // e4
    Success &= (attacks & bb_Square(35)) != 0ULL;  // d5
    Success &= (attacks & bb_Square(42)) != 0ULL;  // c6
    Success &= (attacks & bb_Square(49)) != 0ULL;  // b7
    Success &= (attacks & bb_Square(56)) != 0ULL;  // a8

    return Success;
}

static bool test_bbPseudoAttacksRook_d4() {
    bool Success = true;

    // Rook at d4 (sq=27): file d (7 others) + rank 4 (7 others) = 14 squares
    Bitboard attacks = BB_PseudoAttacks_Rook[27];
    Success &= bb_popcount(attacks) == 14;

    // File d: d1(3), d2(11), d3(19), d5(35), d6(43), d7(51), d8(59)
    Success &= (attacks & bb_Square(3)) != 0ULL;   // d1
    Success &= (attacks & bb_Square(11)) != 0ULL;  // d2
    Success &= (attacks & bb_Square(19)) != 0ULL;  // d3
    Success &= (attacks & bb_Square(35)) != 0ULL;  // d5
    Success &= (attacks & bb_Square(43)) != 0ULL;  // d6
    Success &= (attacks & bb_Square(51)) != 0ULL;  // d7
    Success &= (attacks & bb_Square(59)) != 0ULL;  // d8

    // Rank 4: a4(24), b4(25), c4(26), e4(28), f4(29), g4(30), h4(31)
    Success &= (attacks & bb_Square(24)) != 0ULL;  // a4
    Success &= (attacks & bb_Square(25)) != 0ULL;  // b4
    Success &= (attacks & bb_Square(26)) != 0ULL;  // c4
    Success &= (attacks & bb_Square(28)) != 0ULL;  // e4
    Success &= (attacks & bb_Square(29)) != 0ULL;  // f4
    Success &= (attacks & bb_Square(30)) != 0ULL;  // g4
    Success &= (attacks & bb_Square(31)) != 0ULL;  // h4

    // d4 itself must NOT be in the attack mask
    Success &= (attacks & bb_Square(27)) == 0ULL;

    return Success;
}

static bool test_bbPseudoAttacksRook_a1() {
    bool Success = true;

    // Rook at a1 (sq=0): file a (7) + rank 1 (7) = 14 squares
    Bitboard attacks = BB_PseudoAttacks_Rook[0];
    Success &= bb_popcount(attacks) == 14;

    // File a: a2(8), a3(16), a4(24), a5(32), a6(40), a7(48), a8(56)
    for (int r = 1; r <= 7; r++) {
        int sq = r * 8;
        Success &= (attacks & bb_Square(sq)) != 0ULL;
    }

    // Rank 1: b1(1), c1(2), d1(3), e1(4), f1(5), g1(6), h1(7)
    for (int f = 1; f <= 7; f++) {
        Success &= (attacks & bb_Square(f)) != 0ULL;
    }

    return Success;
}

static bool test_bbPseudoAttacksRook_cornerPopcount() {
    bool Success = true;

    // All corners have 14 attacks (7 along file + 7 along rank)
    Success &= bb_popcount(BB_PseudoAttacks_Rook[0])  == 14;  // a1
    Success &= bb_popcount(BB_PseudoAttacks_Rook[7])  == 14;  // h1
    Success &= bb_popcount(BB_PseudoAttacks_Rook[56]) == 14;  // a8
    Success &= bb_popcount(BB_PseudoAttacks_Rook[63]) == 14;  // h8

    return Success;
}

static bool test_bbPseudoAttacksRook_interiorPopcount() {
    bool Success = true;

    // All interior squares (c3..f6, i.e., files 2-5, ranks 2-5) have 14 attacks
    for (int rank = 1; rank <= 6; rank++) {
        for (int file = 0; file <= 7; file++) {
            int sq = rank * 8 + file;
            if (file >= 1 && file <= 6 && rank >= 1 && rank <= 6) {
                Success &= bb_popcount(BB_PseudoAttacks_Rook[sq]) == 14;
            }
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksBishop_noOffBoardBits() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Bishop[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_lsb(tmp);
            Success &= t >= 0 && t < 64;
            bb_pop_lsb(&tmp);
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksRook_noOffBoardBits() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Rook[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_lsb(tmp);
            Success &= t >= 0 && t < 64;
            bb_pop_lsb(&tmp);
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksBishop_symmetry() {
    bool Success = true;

    // If sq X is in BB_PseudoAttacks_Bishop[Y], then Y must be in BB_PseudoAttacks_Bishop[X]
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Bishop[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_pop_lsb(&tmp);
            Success &= (BB_PseudoAttacks_Bishop[t] & bb_Square(sq)) != 0ULL;
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksRook_symmetry() {
    bool Success = true;

    // If sq X is in BB_PseudoAttacks_Rook[Y], then Y must be in BB_PseudoAttacks_Rook[X]
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = BB_PseudoAttacks_Rook[sq];
        Bitboard tmp = attacks;
        while (tmp) {
            int t = bb_pop_lsb(&tmp);
            Success &= (BB_PseudoAttacks_Rook[t] & bb_Square(sq)) != 0ULL;
        }
    }

    return Success;
}

static bool test_bbPseudoAttacksBishop_noSelf() {
    bool Success = true;

    // No square should attack itself
    for (int sq = 0; sq < 64; ++sq) {
        Success &= (BB_PseudoAttacks_Bishop[sq] & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbPseudoAttacksRook_noSelf() {
    bool Success = true;

    // No square should attack itself
    for (int sq = 0; sq < 64; ++sq) {
        Success &= (BB_PseudoAttacks_Rook[sq] & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbPseudoAttacksBishop_expectedPopcounts() {
    bool Success = true;

    // Corners: 7 attacks each (full diagonal)
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[0])  == 7;  // a1
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[7])  == 7;  // h1
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[56]) == 7;  // a8
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[63]) == 7;  // h8

    // Center squares: 13 attacks each (both diagonals maxed out)
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[27]) == 13;  // d4
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[28]) == 13;  // e4
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[35]) == 13;  // d5
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[36]) == 13;  // e5

    // Edge non-corner squares (e.g., a2=8, h2=15): 7 attacks (6+1 on the two diagonals)
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[8])  == 7;  // a2
    Success &= bb_popcount(BB_PseudoAttacks_Bishop[15]) == 7;  // h2

    return Success;
}

static bool test_bbPseudoAttacksRook_expectedPopcounts() {
    bool Success = true;

    // Corners: 14 attacks each (7+7)
    Success &= bb_popcount(BB_PseudoAttacks_Rook[0])  == 14;  // a1
    Success &= bb_popcount(BB_PseudoAttacks_Rook[7])  == 14;  // h1
    Success &= bb_popcount(BB_PseudoAttacks_Rook[56]) == 14;  // a8
    Success &= bb_popcount(BB_PseudoAttacks_Rook[63]) == 14;  // h8

    // Edge non-corner squares: 14 attacks (7 along file + 7 along rank)
    // Every non-corner square has 7 squares on its file and 7 on its rank.
    Success &= bb_popcount(BB_PseudoAttacks_Rook[8])  == 14;  // a2
    Success &= bb_popcount(BB_PseudoAttacks_Rook[15]) == 14;  // h2
    Success &= bb_popcount(BB_PseudoAttacks_Rook[48]) == 14;  // a7
    Success &= bb_popcount(BB_PseudoAttacks_Rook[55]) == 14;  // h7

    // b1(1), g1(6): 7 on file + 7 on rank = 14
    Success &= bb_popcount(BB_PseudoAttacks_Rook[1])  == 14;  // b1
    Success &= bb_popcount(BB_PseudoAttacks_Rook[6])  == 14;  // g1

    return Success;
}

// --- bb_slidingAttack_bishop() tests ---

static bool test_bbSlidingBishop_emptyBoard() {
    bool Success = true;

    // Bishop at d4 (sq=27) on empty board: should equal max coverage
    Bitboard attacks = bb_slidingAttack_bishop(27, 0ULL);
    Success &= attacks == BB_PseudoAttacks_Bishop[27];
    Success &= bb_popcount(attacks) == 13;

    return Success;
}

static bool test_bbSlidingBishop_blockedForward() {
    bool Success = true;

    // Bishop at d4 (sq=27), blocker on e5 (sq=36)
    // NE diag: e5(36) is included but f6(45) is NOT
    Bitboard occupied = bb_Square(36);
    Bitboard attacks = bb_slidingAttack_bishop(27, occupied);

    // e5 must be in the attack set (can capture)
    Success &= (attacks & bb_Square(36)) != 0ULL;

    // f6 must NOT be in the attack set (blocked by e5)
    Success &= (attacks & bb_Square(45)) == 0ULL;
    // g7 must NOT be in the attack set
    Success &= (attacks & bb_Square(54)) == 0ULL;
    // h8 must NOT be in the attack set
    Success &= (attacks & bb_Square(63)) == 0ULL;

    return Success;
}

static bool test_bbSlidingBishop_blockedSW() {
    bool Success = true;

    // Bishop at d4 (sq=27), blocker on c3 (sq=18)
    // SW diag: c3(18) included, b2(9) and a1(0) excluded
    Bitboard occupied = bb_Square(18);
    Bitboard attacks = bb_slidingAttack_bishop(27, occupied);

    // c3 must be in the attack set
    Success &= (attacks & bb_Square(18)) != 0ULL;

    // b2 must NOT be in the attack set
    Success &= (attacks & bb_Square(9)) == 0ULL;
    // a1 must NOT be in the attack set
    Success &= (attacks & bb_Square(0)) == 0ULL;

    return Success;
}

static bool test_bbSlidingBishop_cornerEmpty() {
    bool Success = true;

    // Bishop at a1 (sq=0) on empty board: covers a1-h8 diagonal (7 squares)
    Bitboard attacks = bb_slidingAttack_bishop(0, 0ULL);
    Success &= bb_popcount(attacks) == 7;
    Success &= attacks == BB_PseudoAttacks_Bishop[0];

    return Success;
}

static bool test_bbSlidingBishop_cornerBlocked() {
    bool Success = true;

    // Bishop at a1 (sq=0), blocker on d4 (sq=27)
    // b2-c3-d4 covered (3 squares, own square excluded), nothing beyond d4
    Bitboard occupied = bb_Square(27);
    Bitboard attacks = bb_slidingAttack_bishop(0, occupied);

    Success &= bb_popcount(attacks) == 3;
    Success &= (attacks & bb_Square(9)) != 0ULL;   // b2
    Success &= (attacks & bb_Square(18)) != 0ULL;  // c3
    Success &= (attacks & bb_Square(27)) != 0ULL;  // d4 (the blocker, capturable)
    Success &= (attacks & bb_Square(36)) == 0ULL;  // e5 blocked
    Success &= (attacks & bb_Square(45)) == 0ULL;  // f6 blocked
    Success &= (attacks & bb_Square(54)) == 0ULL;  // g7 blocked
    Success &= (attacks & bb_Square(63)) == 0ULL;  // h8 blocked

    return Success;
}

static bool test_bbSlidingBishop_h1Empty() {
    bool Success = true;

    // Bishop at h1 (sq=7) on empty board: 7 squares on h1-a8 diagonal
    Bitboard attacks = bb_slidingAttack_bishop(7, 0ULL);
    Success &= bb_popcount(attacks) == 7;
    Success &= attacks == BB_PseudoAttacks_Bishop[7];

    return Success;
}

static bool test_bbSlidingBishop_multipleBlockers() {
    bool Success = true;

    // Bishop at d4 (sq=27), blockers on e5(36) and c3(18)
    Bitboard occupied = bb_Square(36) | bb_Square(18);
    Bitboard attacks = bb_slidingAttack_bishop(27, occupied);

    // Both blockers included (capturable)
    Success &= (attacks & bb_Square(36)) != 0ULL;
    Success &= (attacks & bb_Square(18)) != 0ULL;

    // Beyond e5: f6,g7,h8 all blocked
    Success &= (attacks & bb_Square(45)) == 0ULL;
    Success &= (attacks & bb_Square(54)) == 0ULL;
    Success &= (attacks & bb_Square(63)) == 0ULL;

    // Beyond c3: b2,a1 all blocked
    Success &= (attacks & bb_Square(9)) == 0ULL;
    Success &= (attacks & bb_Square(0)) == 0ULL;

    // Other diags unaffected: anti-diag still has g1,f2,e3,c5,b6,a7
    Success &= (attacks & bb_Square(6)) != 0ULL;   // g1
    Success &= (attacks & bb_Square(13)) != 0ULL;  // f2
    Success &= (attacks & bb_Square(20)) != 0ULL;  // e3
    Success &= (attacks & bb_Square(34)) != 0ULL;  // c5
    Success &= (attacks & bb_Square(41)) != 0ULL;  // b6
    Success &= (attacks & bb_Square(48)) != 0ULL;  // a7

    return Success;
}

static bool test_bbSlidingBishop_allSquares_emptyMatchesMax() {
    bool Success = true;

    // For every square, bishop attacks on empty board must equal max coverage table
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = bb_slidingAttack_bishop(sq, 0ULL);
        Success &= attacks == BB_PseudoAttacks_Bishop[sq];
    }

    return Success;
}

static bool test_bbSlidingBishop_noSelf() {
    bool Success = true;

    // The bishop's own square must never appear in the attack bitboard
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard occupied = bb_Square(sq);  // occupy only own square
        Bitboard attacks = bb_slidingAttack_bishop(sq, occupied);
        Success &= (attacks & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbSlidingBishop_firstOccupiedBlocks() {
    bool Success = true;

    // For every square and every possible single-blocker position,
    // verify that the attack set is a subset of max coverage and doesn't
    // extend past the blocker.
    for (int sq = 0; sq < 64; ++sq) {
        for (int blocker = 0; blocker < 64; ++blocker) {
            if (blocker == sq) continue;
            Bitboard occupied = bb_Square(blocker);
            Bitboard attacks = bb_slidingAttack_bishop(sq, occupied);

            // Must be subset of max coverage
            Success &= (attacks & ~BB_PseudoAttacks_Bishop[sq]) == 0ULL;

            // If blocker is in attacks, no squares beyond it on that diagonal
            if (attacks & bb_Square(blocker)) {
                // Blocker was reached — verify it's the last square on its ray from sq
                int f1 = sq % 8, r1 = sq / 8;
                int f2 = blocker % 8, r2 = blocker / 8;
                int df = f2 - f1, dr = r2 - r1;

                // Continue one step past the blocker
                int bf = f2 + df, br = r2 + dr;
                if (bf >= 0 && bf < 8 && br >= 0 && br < 8) {
                    int nextSq = br * 8 + bf;
                    // Only valid if this step stays on same diagonal
                    if ((df == 0 || (f1 == f2)) || (r1 == r2) ||
                        (df == dr) || (df == -dr)) {
                        Success &= (attacks & bb_Square(nextSq)) == 0ULL;
                    }
                }
            }
        }
    }

    return Success;
}

// --- bb_slidingAttack_rook() tests ---

static bool test_bbSlidingRook_emptyBoard() {
    bool Success = true;

    // Rook at d4 (sq=27) on empty board: should equal max coverage (14 squares)
    Bitboard attacks = bb_slidingAttack_rook(27, 0ULL);
    Success &= attacks == BB_PseudoAttacks_Rook[27];
    Success &= bb_popcount(attacks) == 14;

    return Success;
}

static bool test_bbSlidingRook_blockedNorth() {
    bool Success = true;

    // Rook at d4 (sq=27), blocker on d5 (sq=35)
    // North: d5 included, d6-d8 excluded
    Bitboard occupied = bb_Square(35);
    Bitboard attacks = bb_slidingAttack_rook(27, occupied);

    // d5 must be in the attack set (can capture)
    Success &= (attacks & bb_Square(35)) != 0ULL;

    // d6-d8 must NOT be in the attack set
    Success &= (attacks & bb_Square(43)) == 0ULL;  // d6
    Success &= (attacks & bb_Square(51)) == 0ULL;  // d7
    Success &= (attacks & bb_Square(59)) == 0ULL;  // d8

    return Success;
}

static bool test_bbSlidingRook_blockedEast() {
    bool Success = true;

    // Rook at d4 (sq=27), blocker on e4 (sq=28)
    // East: e4 included, f4-h4 excluded
    Bitboard occupied = bb_Square(28);
    Bitboard attacks = bb_slidingAttack_rook(27, occupied);

    // e4 must be in the attack set
    Success &= (attacks & bb_Square(28)) != 0ULL;

    // f4-h4 must NOT be in the attack set
    Success &= (attacks & bb_Square(29)) == 0ULL;  // f4
    Success &= (attacks & bb_Square(30)) == 0ULL;  // g4
    Success &= (attacks & bb_Square(31)) == 0ULL;  // h4

    return Success;
}

static bool test_bbSlidingRook_cornerEmpty() {
    bool Success = true;

    // Rook at a1 (sq=0) on empty board: 14 squares (7 up + 7 right)
    Bitboard attacks = bb_slidingAttack_rook(0, 0ULL);
    Success &= bb_popcount(attacks) == 14;
    Success &= attacks == BB_PseudoAttacks_Rook[0];

    return Success;
}

static bool test_bbSlidingRook_multipleBlockers() {
    bool Success = true;

    // Rook at d4 (sq=27), blockers on d5(35) and e4(28)
    Bitboard occupied = bb_Square(35) | bb_Square(28);
    Bitboard attacks = bb_slidingAttack_rook(27, occupied);

    // Both blockers included (capturable)
    Success &= (attacks & bb_Square(35)) != 0ULL;
    Success &= (attacks & bb_Square(28)) != 0ULL;

    // Beyond d5: d6,d7,d8 blocked
    Success &= (attacks & bb_Square(43)) == 0ULL;
    Success &= (attacks & bb_Square(51)) == 0ULL;
    Success &= (attacks & bb_Square(59)) == 0ULL;

    // Beyond e4: f4,g4,h4 blocked
    Success &= (attacks & bb_Square(29)) == 0ULL;
    Success &= (attacks & bb_Square(30)) == 0ULL;
    Success &= (attacks & bb_Square(31)) == 0ULL;

    // South direction unaffected: d1-d3 still reachable
    Success &= (attacks & bb_Square(19)) != 0ULL;  // d3
    Success &= (attacks & bb_Square(11)) != 0ULL;  // d2
    Success &= (attacks & bb_Square(3))  != 0ULL;  // d1

    // West direction unaffected: c4,a4 still reachable
    Success &= (attacks & bb_Square(26)) != 0ULL;  // c4
    Success &= (attacks & bb_Square(25)) != 0ULL;  // b4
    Success &= (attacks & bb_Square(24)) != 0ULL;  // a4

    return Success;
}

static bool test_bbSlidingRook_h8Empty() {
    bool Success = true;

    // Rook at h8 (sq=63) on empty board: 14 squares (7 left + 7 down)
    Bitboard attacks = bb_slidingAttack_rook(63, 0ULL);
    Success &= bb_popcount(attacks) == 14;
    Success &= attacks == BB_PseudoAttacks_Rook[63];

    return Success;
}

static bool test_bbSlidingRook_allSquares_emptyMatchesMax() {
    bool Success = true;

    // For every square, rook attacks on empty board must equal max coverage table
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard attacks = bb_slidingAttack_rook(sq, 0ULL);
        Success &= attacks == BB_PseudoAttacks_Rook[sq];
    }

    return Success;
}

static bool test_bbSlidingRook_noSelf() {
    bool Success = true;

    // The rook's own square must never appear in the attack bitboard
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard occupied = bb_Square(sq);
        Bitboard attacks = bb_slidingAttack_rook(sq, occupied);
        Success &= (attacks & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbSlidingRook_firstOccupiedBlocks() {
    bool Success = true;

    // For every square and every possible single-blocker position,
    // verify that the attack set is a subset of max coverage and doesn't
    // extend past the blocker.
    for (int sq = 0; sq < 64; ++sq) {
        for (int blocker = 0; blocker < 64; ++blocker) {
            if (blocker == sq) continue;
            Bitboard occupied = bb_Square(blocker);
            Bitboard attacks = bb_slidingAttack_rook(sq, occupied);

            // Must be subset of max coverage
            Success &= (attacks & ~BB_PseudoAttacks_Rook[sq]) == 0ULL;

            // If blocker is in attacks, no squares beyond it on that ray from sq
            if (attacks & bb_Square(blocker)) {
                int f1 = sq % 8, r1 = sq / 8;
                int f2 = blocker % 8, r2 = blocker / 8;
                int df = f2 - f1, dr = r2 - r1;

                // Continue one step past the blocker
                int bf = f2 + df, br = r2 + dr;
                if (bf >= 0 && bf < 8 && br >= 0 && br < 8) {
                    int nextSq = br * 8 + bf;
                    // Only valid if this step stays on same rank/file
                    if ((df == 0 || (f1 == f2)) || (r1 == r2)) {
                        Success &= (attacks & bb_Square(nextSq)) == 0ULL;
                    }
                }
            }
        }
    }

    return Success;
}

// --- bb_slidingAttack_queen() tests ---

static bool test_bbSlidingQueen_emptyBoard_d4() {
    bool Success = true;

    // Queen at d4 (sq=27) on empty board: bishop(13) + rook(14) = 27 squares
    Bitboard attacks = bb_slidingAttack_queen(27, 0ULL);
    Success &= bb_popcount(attacks) == 27;

    // Must equal union of bishop and rook attacks
    Bitboard expected = bb_slidingAttack_bishop(27, 0ULL) | bb_slidingAttack_rook(27, 0ULL);
    Success &= attacks == expected;

    return Success;
}

static bool test_bbSlidingQueen_emptyBoard_a1() {
    bool Success = true;

    // Queen at a1 (sq=0): bishop(7) + rook(14) = 21 squares
    Bitboard attacks = bb_slidingAttack_queen(0, 0ULL);
    Success &= bb_popcount(attacks) == 21;

    Bitboard expected = bb_slidingAttack_bishop(0, 0ULL) | bb_slidingAttack_rook(0, 0ULL);
    Success &= attacks == expected;

    return Success;
}

static bool test_bbSlidingQueen_blockedDiagonalAndRank() {
    bool Success = true;

    // Queen at d4 (sq=27). Blocker on c5(34) blocks NW diag for bishop.
    // Blocker on d6(43) blocks north for rook.
    // These blockers are on different rays so don't interfere.
    Bitboard occupied = bb_Square(34) | bb_Square(43);
    Bitboard attacks = bb_slidingAttack_queen(27, occupied);

    // Bishop NW diag: c5(34) capturable, b6(41) and a7(48) blocked
    Success &= (attacks & bb_Square(34)) != 0ULL;   // c5 capturable
    Success &= (attacks & bb_Square(41)) == 0ULL;   // b6 blocked by c5
    Success &= (attacks & bb_Square(48)) == 0ULL;   // a7 blocked by c5

    // Rook north: d5(35) reachable, d6(43) capturable, d7-d8 blocked
    Success &= (attacks & bb_Square(35)) != 0ULL;   // d5 reachable
    Success &= (attacks & bb_Square(43)) != 0ULL;   // d6 capturable
    Success &= (attacks & bb_Square(51)) == 0ULL;   // d7 blocked by d6
    Success &= (attacks & bb_Square(59)) == 0ULL;   // d8 blocked by d6

    return Success;
}

static bool test_bbSlidingQueen_equalsUnionAllSquares() {
    bool Success = true;

    // For every square on empty board, queen attacks must equal bishop | rook
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard qAtk = bb_slidingAttack_queen(sq, 0ULL);
        Bitboard expected = bb_slidingAttack_bishop(sq, 0ULL) | bb_slidingAttack_rook(sq, 0ULL);
        Success &= qAtk == expected;
    }

    return Success;
}

static bool test_bbSlidingQueen_equalsUnionWithOccupancy() {
    bool Success = true;

    // Test with various occupancy patterns
    Bitboard occupancies[] = {
        0ULL,                                          // empty
        bb_Square(35),                                 // single center piece
        bb_Square(0) | bb_Square(63),               // two corners
        BB_RANK_1 | BB_RANK_8,                        // two ranks full
        BB_FILE_A | BB_FILE_H,                        // two files full
        0xAAAAAAAAAAAAAAAAULL,                         // checkerboard-ish
        0x5555555555555555ULL,                      // alternate checkerboard
    };
    int n = sizeof(occupancies) / sizeof(occupancies[0]);

    for (int occIdx = 0; occIdx < n; ++occIdx) {
        Bitboard occ = occupancies[occIdx];
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard qAtk = bb_slidingAttack_queen(sq, occ);
            Bitboard expected = bb_slidingAttack_bishop(sq, occ) | bb_slidingAttack_rook(sq, occ);
            Success &= qAtk == expected;
        }
    }

    return Success;
}

static bool test_bbSlidingQueen_noSelf() {
    bool Success = true;

    // Queen's own square must never appear in attack bitboard
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard occupied = bb_Square(sq);
        Bitboard attacks = bb_slidingAttack_queen(sq, occupied);
        Success &= (attacks & bb_Square(sq)) == 0ULL;
    }

    return Success;
}

static bool test_bbSlidingQueen_subsetOfMaxCoverage() {
    bool Success = true;

    // Queen attacks with any occupancy must be subset of max coverage (bishop | rook pseudo-attacks)
    for (int sq = 0; sq < 64; ++sq) {
        Bitboard maxCoverage = BB_PseudoAttacks_Bishop[sq] | BB_PseudoAttacks_Rook[sq];
        Bitboard occupied = 0xACEFACEACEACEACEULL;
        Bitboard attacks = bb_slidingAttack_queen(sq, occupied);
        Success &= (attacks & ~maxCoverage) == 0ULL;
    }

    return Success;
}

static bool test_bbBishopMagic_allSquaresHaveMagic() {
    bool Success = true;

    // A bishop is always on at least one diagonal, so every square gets a real
    // (non-zero) magic number.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= BB_MagicBishop[sq] != 0ULL;
    }

    return Success;
}

static bool test_bbBishopRelOcc_isRayCoverage() {
    bool Success = true;

    // For a bishop the relevant occupancy is exactly its full ray coverage.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= BB_RelativeOcc_Bishop[sq] == BB_PseudoAttacks_Bishop[sq];
    }

    return Success;
}

static bool test_bbBishopMagic_maxRelevantBits() {
    bool Success = true;

    // The lookup table is 2^13; no square may need more relevant bits than that.
    for (int sq = 0; sq < 64; ++sq) {
        int k = bb_popcount(BB_RelativeOcc_Bishop[sq]);
        Success &= k <= 13;
    }

    return Success;
}

static bool test_bbBishopMagic_emptyBoardMatchesMax() {
    bool Success = true;

    // No relevant occupancy -> full diagonal ray coverage, same as the naive
    // sliding attack on an empty board.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= bb_bishopAttacks(sq, 0ULL) == BB_PseudoAttacks_Bishop[sq];
    }

    return Success;
}

static bool test_bbBishopMagic_noSelf() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard self = ((Bitboard)1 << sq);
        Bitboard occs[] = {0ULL, 0xFFFFFFFFFFFFFFFFULL, self};
        for (int i = 0; i < 3; ++i) {
            Success &= (bb_bishopAttacks(sq, occs[i]) & self) == 0ULL;
        }
    }

    return Success;
}

static bool test_bbBishopMagic_matchesNaive() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard full = 0xFFFFFFFFFFFFFFFFULL;

        // Empty and fully-occupied boards.
        Success &= bb_bishopAttacks(sq, 0ULL) == bb_slidingAttack_bishop(sq, 0ULL);
        Success &= bb_bishopAttacks(sq, full) == bb_slidingAttack_bishop(sq, full);

        // Every single-square occupant.
        for (int i = 0; i < 64; ++i) {
            Bitboard occ = ((Bitboard)1 << i);
            Success &= bb_bishopAttacks(sq, occ) == bb_slidingAttack_bishop(sq, occ);
        }

        // A spread of multi-bit occupancies (deterministic submasks of relOcc),
        // exercising the higher-index entries of the lookup table.
        for (int j = 0; j < 16; ++j) {
            Bitboard occ = ((Bitboard)sq * 2654435761ULL + (Bitboard)j * 40503ULL + 12345ULL)
                * 6364136223846793005ULL + 1442695040888963407ULL;
            occ &= BB_RelativeOcc_Bishop[sq];
            Success &= bb_bishopAttacks(sq, occ) == bb_slidingAttack_bishop(sq, occ);
        }
    }

    return Success;
}

static bool test_bbRookMagic_allSquaresHaveMagic() {
    bool Success = true;

    // A rook is always on at least one rank and one file, so every square gets
    // a real (non-zero) magic number.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= BB_MagicRook[sq] != 0ULL;
    }

    return Success;
}

static bool test_bbRookRelOcc_isRayCoverage() {
    bool Success = true;

    // For a rook the relevant occupancy is exactly its full rank/file ray
    // coverage.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= BB_RelativeOcc_Rook[sq] == BB_PseudoAttacks_Rook[sq];
    }

    return Success;
}

static bool test_bbRookMagic_maxRelevantBits() {
    bool Success = true;

    // A rook sees 7 squares on its rank + 7 on its file = 14 relevant bits,
    // uniformly for every square; the table is 2^14.
    for (int sq = 0; sq < 64; ++sq) {
        int k = bb_popcount(BB_RelativeOcc_Rook[sq]);
        Success &= k <= 14;
    }

    return Success;
}

static bool test_bbRookMagic_emptyBoardMatchesMax() {
    bool Success = true;

    // No relevant occupancy -> full rank/file ray coverage, same as the naive
    // sliding attack on an empty board.
    for (int sq = 0; sq < 64; ++sq) {
        Success &= bb_rookAttacks(sq, 0ULL) == BB_PseudoAttacks_Rook[sq];
    }

    return Success;
}

static bool test_bbRookMagic_noSelf() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard self = ((Bitboard)1 << sq);
        Bitboard occs[] = {0ULL, 0xFFFFFFFFFFFFFFFFULL, self};
        for (int i = 0; i < 3; ++i) {
            Success &= (bb_rookAttacks(sq, occs[i]) & self) == 0ULL;
        }
    }

    return Success;
}

static bool test_bbRookMagic_matchesNaive() {
    bool Success = true;

    for (int sq = 0; sq < 64; ++sq) {
        Bitboard full = 0xFFFFFFFFFFFFFFFFULL;

        // Empty and fully-occupied boards.
        Success &= bb_rookAttacks(sq, 0ULL) == bb_slidingAttack_rook(sq, 0ULL);
        Success &= bb_rookAttacks(sq, full) == bb_slidingAttack_rook(sq, full);

        // Every single-square occupant.
        for (int i = 0; i < 64; ++i) {
            Bitboard occ = ((Bitboard)1 << i);
            Success &= bb_rookAttacks(sq, occ) == bb_slidingAttack_rook(sq, occ);
        }

        // A spread of multi-bit occupancies (deterministic submasks of relOcc),
        // exercising the higher-index entries of the lookup table.
        for (int j = 0; j < 16; ++j) {
            Bitboard occ = ((Bitboard)sq * 2654435761ULL + (Bitboard)j * 40503ULL + 12345ULL)
                * 6364136223846793005ULL + 1442695040888963407ULL;
            occ &= BB_RelativeOcc_Rook[sq];
            Success &= bb_rookAttacks(sq, occ) == bb_slidingAttack_rook(sq, occ);
        }
    }

    return Success;
}

extern void bb_initPseudoAttacks(void);
extern void bb_initMagics_bishop(void);
extern Bitboard bb_bishopAttacks(int sq, Bitboard occupied);
extern void bb_initMagics_rook(void);
extern Bitboard bb_rookAttacks(int sq, Bitboard occupied);

int main() {
    bool Success = true;

    bb_initPseudoAttacks();
    bb_initMagics_bishop();
    bb_initMagics_rook();

Success &= test_bbPseudoAttacksKnight_corners();
    Success &= test_bbPseudoAttacksKnight_center();
    Success &= test_bbPseudoAttacksKnight_onBoard();
    Success &= test_bbPseudoAttacksKnight_symmetry();
    Success &= test_bbPseudoAttacksKnight_expectedPopcounts();
    Success &= test_bbPseudoAttacksKing_corners();
    Success &= test_bbPseudoAttacksKing_center();
    Success &= test_bbPseudoAttacksKing_edgeSquares();
    Success &= test_bbPseudoAttacksKing_nearCorner();
    Success &= test_bbPseudoAttacksKing_onBoard();
    Success &= test_bbPseudoAttacksKing_symmetry();
    Success &= test_bbPseudoAttacksKing_noSelf();
    Success &= test_bbPseudoAttacksKing_expectedPopcounts();
    Success &= test_bbPawnAttacks_whiteCenter();
    Success &= test_bbPawnAttacks_whiteEdgeH();
    Success &= test_bbPawnAttacks_whiteEdgeA();
    Success &= test_bbPawnAttacks_blackCenter();
    Success &= test_bbPawnAttacks_blackEdgeH();
    Success &= test_bbPawnAttacks_blackEdgeA();
    Success &= test_bbPawnAttacks_symmetry();
    Success &= test_bbPawnAttacks_noOffBoardBits();
    Success &= test_bbPawnAttacks_whiteRank1();
    Success &= test_bbPawnAttacks_whiteRank8_none();
    Success &= test_bbPawnAttacks_blackRank8_none();
    Success &= test_bbPawnAttacks_allPopcounts();
    Success &= test_bbPawnPushes_whiteE2_doublePush();
    Success &= test_bbPawnPushes_whiteE3_singlePush();
    Success &= test_bbPawnPushes_whiteE7_promoRank();
    Success &= test_bbPawnPushes_whiteA1_rank0();
    Success &= test_bbPawnPushes_whiteRank8_none();
    Success &= test_bbPawnPushes_blackE7_doublePush();
    Success &= test_bbPawnPushes_blackE6_singlePush();
    Success &= test_bbPawnPushes_blackA8_rank0();
    Success &= test_bbPawnPushes_hFile_white();
    Success &= test_bbPawnPushes_hFile_black();
    Success &= test_bbPawnPushes_noOffBoardBits();
    Success &= test_bbPawnPushes_allPopcounts();
    Success &= test_bbPawnPushes_symmetry();
    Success &= test_bbPawnPushes_sameFile();
    Success &= test_shift();
    Success &= test_bbSquare();
    Success &= test_bbPopcount();
    Success &= test_bbLsbMsb();
    Success &= test_bbPopLsb();
    Success &= test_bbMoreThanOne();
    Success &= test_bbNextBit();
    Success &= test_CoordToBB();
    Success &= test_EnPassant_initNone();
    Success &= test_EnPassant_setGet();
    Success &= test_EnPassant_validSquares();
    Success &= test_EnPassant_noneMeansNoRight();
    Success &= test_BitboardState_zeroInit();
    Success &= test_BitboardState_setPiece();
    Success &= test_BitboardState_updateOccupancy();
    Success &= test_BitboardState_fullPosition();
    Success &= test_Occupancy_twoPieces();
    Success &= test_Castling_zeroInit();
    Success &= test_Castling_setWK();
    Success &= test_Castling_allRights();
    Success &= test_Castling_clearBK();
    Success &= test_Castling_roundTrip();
    Success &= test_Castling_clearUnset();
    Success &= test_Castling_constants();
    Success &= test_bbParseFEN_startPosition();
    Success &= test_bbParseFEN_enPassant();
    Success &= test_bbParseFEN_reducedCastling();
    Success &= test_bbParseFEN_noCastling();
    Success &= test_bbParseFEN_activeColorBlack();
    Success &= test_bbParseFEN_halfmoveFullmove();
    Success &= test_bbParseFEN_kingsIndication();
    Success &= test_bbParseFEN_midgame();
    Success &= test_bbFenToString_startPosition();
    Success &= test_bbFenToString_enPassant();
    Success &= test_bbFenToString_reducedCastling();
    Success &= test_bbFenToString_emptyBoard();
    Success &= test_bbFenToString_blackToMove();
    Success &= test_bbFenToString_halfmoveFullmove();
    Success &= test_bbFenToString_midgame();
    Success &= test_bbFenToString_roundtrip();
    Success &= test_bbPrintBoard_startPosition();
    Success &= test_bbPrintBoard_emptyBoard();
    Success &= test_bbPrintBoard_enPassantIndicator();
    Success &= test_bbPrintBoard_activeColor();
    Success &= test_bbPseudoAttacksBishop_d4();
    Success &= test_bbPseudoAttacksBishop_a1();
    Success &= test_bbPseudoAttacksBishop_h1();
    Success &= test_bbPseudoAttacksRook_d4();
    Success &= test_bbPseudoAttacksRook_a1();
    Success &= test_bbPseudoAttacksRook_cornerPopcount();
    Success &= test_bbPseudoAttacksRook_interiorPopcount();
    Success &= test_bbPseudoAttacksBishop_noOffBoardBits();
    Success &= test_bbPseudoAttacksRook_noOffBoardBits();
    Success &= test_bbPseudoAttacksBishop_symmetry();
    Success &= test_bbPseudoAttacksRook_symmetry();
    Success &= test_bbPseudoAttacksBishop_noSelf();
    Success &= test_bbPseudoAttacksRook_noSelf();
    Success &= test_bbPseudoAttacksBishop_expectedPopcounts();
    Success &= test_bbPseudoAttacksRook_expectedPopcounts();
    Success &= test_bbSlidingBishop_emptyBoard();
    Success &= test_bbSlidingBishop_blockedForward();
    Success &= test_bbSlidingBishop_blockedSW();
    Success &= test_bbSlidingBishop_cornerEmpty();
    Success &= test_bbSlidingBishop_cornerBlocked();
    Success &= test_bbSlidingBishop_h1Empty();
    Success &= test_bbSlidingBishop_multipleBlockers();
    Success &= test_bbSlidingBishop_allSquares_emptyMatchesMax();
    Success &= test_bbSlidingBishop_noSelf();
    Success &= test_bbSlidingBishop_firstOccupiedBlocks();
    Success &= test_bbSlidingRook_emptyBoard();
    Success &= test_bbSlidingRook_blockedNorth();
    Success &= test_bbSlidingRook_blockedEast();
    Success &= test_bbSlidingRook_cornerEmpty();
    Success &= test_bbSlidingRook_multipleBlockers();
    Success &= test_bbSlidingRook_h8Empty();
    Success &= test_bbSlidingRook_allSquares_emptyMatchesMax();
    Success &= test_bbSlidingRook_noSelf();
    Success &= test_bbSlidingRook_firstOccupiedBlocks();
    Success &= test_bbSlidingQueen_emptyBoard_d4();
    Success &= test_bbSlidingQueen_emptyBoard_a1();
    Success &= test_bbSlidingQueen_blockedDiagonalAndRank();
    Success &= test_bbSlidingQueen_equalsUnionAllSquares();
    Success &= test_bbSlidingQueen_equalsUnionWithOccupancy();
    Success &= test_bbSlidingQueen_noSelf();
    Success &= test_bbSlidingQueen_subsetOfMaxCoverage();
    Success &= test_bbBishopMagic_allSquaresHaveMagic();
    Success &= test_bbBishopRelOcc_isRayCoverage();
    Success &= test_bbBishopMagic_maxRelevantBits();
    Success &= test_bbBishopMagic_emptyBoardMatchesMax();
    Success &= test_bbBishopMagic_noSelf();
    Success &= test_bbBishopMagic_matchesNaive();
    Success &= test_bbRookMagic_allSquaresHaveMagic();
    Success &= test_bbRookRelOcc_isRayCoverage();
    Success &= test_bbRookMagic_maxRelevantBits();
    Success &= test_bbRookMagic_emptyBoardMatchesMax();
    Success &= test_bbRookMagic_noSelf();
    Success &= test_bbRookMagic_matchesNaive();
    assert(Success);

    return !Success;
}
