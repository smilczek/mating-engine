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
    assert(Success);

    return !Success;
}
