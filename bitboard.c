typedef unsigned long long Bitboard;
static const Bitboard BB_RANK_1 = 0xFFULL;
static const Bitboard BB_RANK_2 = BB_RANK_1 << 8 * 1;
static const Bitboard BB_RANK_3 = BB_RANK_1 << 8 * 2;
static const Bitboard BB_RANK_4 = BB_RANK_1 << 8 * 3;
static const Bitboard BB_RANK_5 = BB_RANK_1 << 8 * 4;
static const Bitboard BB_RANK_6 = BB_RANK_1 << 8 * 5;
static const Bitboard BB_RANK_7 = BB_RANK_1 << 8 * 6;
static const Bitboard BB_RANK_8 = BB_RANK_1 << 8 * 7;

static const Bitboard BB_FILE_A = 0x0101010101010101ULL;
static const Bitboard BB_FILE_B = BB_FILE_A << 1;
static const Bitboard BB_FILE_C = BB_FILE_A << 2;
static const Bitboard BB_FILE_D = BB_FILE_A << 3;
static const Bitboard BB_FILE_E = BB_FILE_A << 4;
static const Bitboard BB_FILE_F = BB_FILE_A << 5;
static const Bitboard BB_FILE_G = BB_FILE_A << 6;
static const Bitboard BB_FILE_H = BB_FILE_A << 7;

typedef enum {
    DIR_NORTH = 8,
    DIR_EAST = 1,
    DIR_SOUTH = -DIR_NORTH,
    DIR_WEST = -DIR_EAST,
    DIR_NORTHEAST = DIR_NORTH + DIR_EAST,
    DIR_NORTHWEST = DIR_NORTH + DIR_WEST,
    DIR_SOUTHEAST = -DIR_NORTHWEST,
    DIR_SOUTHWEST = -DIR_NORTHEAST
} Direction;

typedef struct {
    Bitboard King;
    Bitboard Queens;
    Bitboard Rooks;
    Bitboard Knights;
    Bitboard Bishops;
    Bitboard Pawns;
} Pieces;

typedef struct {
    Pieces White;
    Pieces Black;

    bool BlackToMove;

    // Castling rights
    bool CR_WK; // White King
    bool CR_WQ; // White Queen
    bool CR_BK; // Black King
    bool CR_BQ; // Black Queen

    // En passant target
    Bitboard EnPassant;

    // 50-move rule
    int HalfmoveClock;

    // Starts at 0. (Normally starts at 1, stupid,
    // I'd rather keep ZII, add 1 for display)
    // Increments after black's move.
    int FullmoveNumber;
} BitboardState;

Bitboard bb_shift(Bitboard B, Direction D) {
    // I tried to be a smartass here, but decided to prioritise readability.
    return D == DIR_NORTH     ? B << 8 :
           D == DIR_SOUTH     ? B >> 8 :
           D == DIR_EAST      ? (B & ~BB_FILE_H) << 1 :
           D == DIR_WEST      ? (B & ~BB_FILE_A) >> 1 :
           D == DIR_NORTHEAST ? (B & ~BB_FILE_H) << 9 :
           D == DIR_NORTHWEST ? (B & ~BB_FILE_A) << 7 :
           D == DIR_SOUTHEAST ? (B & ~BB_FILE_H) >> 7 :
           D == DIR_SOUTHWEST ? (B & ~BB_FILE_A) >> 9 : 0ULL;
}

int bb_popcount(Bitboard b) {
    return __builtin_popcountll(b);
}

Bitboard bb_Square(int sq) {
    assert(sq >= 0 && sq < 64);
    return 1ULL << sq;
}

int bb_lsb(Bitboard b) {
    assert(b != 0);
    return __builtin_ctzll(b);
}

int bb_msb(Bitboard b) {
    assert(b != 0);
    return 63 ^ __builtin_clzll(b);
}

int bb_pop_lsb(Bitboard *b) {
    assert(*b != 0);
    const int sq = __builtin_ctzll(*b);
    *b &= ~((Bitboard)1 << sq);
    return sq;
}

bool bb_moreThanOne(Bitboard b) {
    return (b & (b - 1)) != 0;
}

int bb_nextBit(Bitboard *remaining) {
    if (*remaining == 0) return -1;
    return bb_pop_lsb(remaining);
}

Bitboard ch_CoordToBB(Coord C) {
    return 1ULL << (C.Rank * BOARDSIZE + C.File);
}
