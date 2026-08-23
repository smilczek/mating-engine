#include <assert.h>
#include <string.h>
#include <stdio.h>

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

typedef enum {
    WHITE = 0,
    BLACK = 1
} Color;

typedef enum {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
} PieceType;

#define BB_CASTLE_WK  (1 << 0)  // White king-side
#define BB_CASTLE_WQ  (1 << 1)  // White queen-side
#define BB_CASTLE_BK  (1 << 2)  // Black king-side
#define BB_CASTLE_BQ  (1 << 3)  // Black queen-side

typedef struct {
    Bitboard Pieces[2][6];   // [color][piece_type] — 12 piece bitboards
    Bitboard Occupancy[2];     // [color] — all pieces of that color
    Bitboard AllPieces;          // all pieces (union of both colors)
    Bitboard Blocked;             // either color (same as AllPieces, conceptually "squares blocked by any piece")
    int EnPassant;                // en passant target square (0-63, or -1 for no EP)
    unsigned char Castling;       // 4-bit castling rights bitmask
    Color ActiveColor;              // side to move (WHITE or BLACK)
    unsigned char HalfmoveClock;   // 50-move rule counter
    unsigned char FullmoveNumber;   // game move number (starts at 0, increments after black's move)
} BitboardState;

static inline int bb_hasCastleRight(BitboardState *s, int rightMask) {
    return (s->Castling & rightMask) != 0;
}

static inline void bb_setCastleRight(BitboardState *s, int rightMask) {
    s->Castling |= rightMask;
}

static inline void bb_clearCastleRight(BitboardState *s, int rightMask) {
    s->Castling &= ~rightMask;
}

static inline void bb_updateOccupancy(BitboardState *s) {
    for (int c = 0; c < 2; c++) {
        s->Occupancy[c] = 0;
        for (int pt = 0; pt < 6; pt++)
            s->Occupancy[c] |= s->Pieces[c][pt];
    }
    s->AllPieces = s->Occupancy[WHITE] | s->Occupancy[BLACK];
    s->Blocked = s->AllPieces;
}

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

// Helper: convert piece character to PieceType
static int bb_pieceCharToType(char c) {
    switch (c) {
        case 'p': case 'P': return PAWN;
        case 'n': case 'N': return KNIGHT;
        case 'b': case 'B': return BISHOP;
        case 'r': case 'R': return ROOK;
        case 'q': case 'Q': return QUEEN;
        case 'k': case 'K': return KING;
        default: return -1;
    }
}

// Parse algebraic square (e.g. "e3") to 0-63 index
static int bb_parseSquare(const char *sq) {
    int file = sq[0] - 'a';
    int rank = sq[1] - '1';
    return rank * 8 + file;
}

// Parse FEN string and populate BitboardState
void bb_parseFEN(BitboardState *s, const char *fen) {
    // Zero out entire state
    memset(s, 0, sizeof(BitboardState));
    s->EnPassant = -1;
    s->ActiveColor = WHITE;

    // --- Parse board section ---
    int rank = 7;  // Start at rank 7 (top), go down to 0
    int file = 0;   // Start at file A (0), go right to H (7)

    while (*fen != ' ' && *fen != '\0') {
        char c = *fen++;
        if (c >= '1' && c <= '8') {
            // Skip empty squares
            file += c - '0';
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            // Place piece
            int color = (c >= 'A' && c <= 'Z') ? WHITE : BLACK;
            int type = bb_pieceCharToType(c);
            if (type >= 0) {
                int sq = rank * 8 + file;
                s->Pieces[color][type] |= bb_Square(sq);
            }
            file++;
        } else if (c == '/') {
            // End of rank
            rank--;
            file = 0;
        }
    }

    // --- Parse active color ---
    fen++;  // skip space
    if (*fen == 'b') {
        s->ActiveColor = BLACK;
    }

    // --- Parse castling rights ---
    fen++;  // skip space
    while (*(++fen) != ' ' && *fen != '\0') {
        switch (*fen) {
            case 'K': bb_setCastleRight(s, BB_CASTLE_WK); break;
            case 'Q': bb_setCastleRight(s, BB_CASTLE_WQ); break;
            case 'k': bb_setCastleRight(s, BB_CASTLE_BK); break;
            case 'q': bb_setCastleRight(s, BB_CASTLE_BQ); break;
            default: break;  // '-' or other chars ignored
        }
    }

    // --- Parse en passant target square ---
    fen++;  // skip space
    if (*fen != '-') {
        s->EnPassant = bb_parseSquare(fen);
        fen += 2;  // skip the two-character square name
    } else {
        fen++;  // skip '-'
    }

    // --- Parse halfmove clock ---
    fen++;  // skip space
    s->HalfmoveClock = 0;
    while (*fen != ' ' && *fen != '\0') {
        s->HalfmoveClock = s->HalfmoveClock * 10 + (*fen++ - '0');
    }

    // --- Parse fullmove number ---
    fen++;  // skip space
    s->FullmoveNumber = 0;
    while (*fen >= '0' && *fen <= '9') {
        s->FullmoveNumber = s->FullmoveNumber * 10 + (*fen++ - '0');
    }

    // Update occupancy bitboards from piece bitboards
    bb_updateOccupancy(s);
}

// Serialize BitboardState back to FEN string
char *bb_fenToString(BitboardState *s, char *buf, int bufSize) {
    int pos = 0;

    // --- Board section ---
    const char PieceChars[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};

    for (int rank = 7; rank >= 0; rank--) {
        int emptyRun = 0;

        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            Bitboard sqBB = bb_Square(sq);

            bool found = false;
            for (int color = 0; color < 2; color++) {
                for (int pt = 0; pt < 6; pt++) {
                    if (s->Pieces[color][pt] & sqBB) {
                        char c = PieceChars[pt];
                        if (color == BLACK) {
                            c = c - 'A' + 'a';
                        }
                        if (emptyRun > 0) {
                            pos += snprintf(buf + pos, bufSize - pos, "%d", emptyRun);
                            emptyRun = 0;
                        }
                        pos += snprintf(buf + pos, bufSize - pos, "%c", c);
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }

            if (!found) {
                emptyRun++;
            }
        }

        if (emptyRun > 0) {
            pos += snprintf(buf + pos, bufSize - pos, "%d", emptyRun);
        }

        if (rank > 0) {
            pos += snprintf(buf + pos, bufSize - pos, "/");
        }
    }

    // --- Active color ---
    pos += snprintf(buf + pos, bufSize - pos, " %c", s->ActiveColor == WHITE ? 'w' : 'b');

    // --- Castling ---
    pos += snprintf(buf + pos, bufSize - pos, " ");
    bool hasCastling = false;
    if (bb_hasCastleRight(s, BB_CASTLE_WK)) {
        pos += snprintf(buf + pos, bufSize - pos, "K");
        hasCastling = true;
    }
    if (bb_hasCastleRight(s, BB_CASTLE_WQ)) {
        pos += snprintf(buf + pos, bufSize - pos, "Q");
        hasCastling = true;
    }
    if (bb_hasCastleRight(s, BB_CASTLE_BK)) {
        pos += snprintf(buf + pos, bufSize - pos, "k");
        hasCastling = true;
    }
    if (bb_hasCastleRight(s, BB_CASTLE_BQ)) {
        pos += snprintf(buf + pos, bufSize - pos, "q");
        hasCastling = true;
    }
    if (!hasCastling) {
        pos += snprintf(buf + pos, bufSize - pos, "-");
    }

    // --- En passant ---
    pos += snprintf(buf + pos, bufSize - pos, " ");
    if (s->EnPassant >= 0) {
        int file = s->EnPassant % 8;
        int rank = s->EnPassant / 8;
        pos += snprintf(buf + pos, bufSize - pos, "%c%d", 'a' + file, rank + 1);
    } else {
        pos += snprintf(buf + pos, bufSize - pos, "-");
    }

    // --- Halfmove clock ---
    pos += snprintf(buf + pos, bufSize - pos, " %u", s->HalfmoveClock);

    // --- Fullmove number ---
    pos += snprintf(buf + pos, bufSize - pos, " %u", s->FullmoveNumber);

    return buf;
}

// Precomputed knight attack bitboards for each square (64 entries)
static Bitboard BB_PseudoAttacks_Knight[64];

// Knight move offsets: (file_delta, rank_delta) pairs
// ±1,±2 and ±2,±1 in flat 8-wide board indexing
static const int KnightOffsets[8][2] = {
    {-1, -2}, {1, -2},
    {-2, -1}, {-2, 1},
    {2, -1},  {2, 1},
    {-1, 2},  {1, 2}
};

// Precomputed king attack bitboards for each square (64 entries)
static Bitboard BB_PseudoAttacks_King[64];

// Precomputed pawn attack bitboards per color per square [color][square]
static Bitboard BB_PawnAttacks[2][64];

// Precomputed legal pawn push squares per color per square [color][square]
static Bitboard BB_PawnPushes[2][64];

// Precomputed bishop pseudo-attack bitboards for each square (max coverage on empty board)
static Bitboard BB_PseudoAttacks_Bishop[64];

// Precomputed rook pseudo-attack bitboards for each square (max coverage on empty board)
static Bitboard BB_PseudoAttacks_Rook[64];

// King move offsets: all 8 adjacent squares (file_delta, rank_delta)
static const int KingOffsets[8][2] = {
    {-1, -1}, {0, -1}, {1, -1},
    {-1,  0},         {1,  0},
    {-1,  1}, {0,  1}, {1,  1}
};

void bb_initPseudoAttacks(void) {
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;
        Bitboard attacks = 0ULL;

        for (int i = 0; i < 8; ++i) {
            int nf = file + KnightOffsets[i][0];
            int nr = rank + KnightOffsets[i][1];

            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                int targetSq = nr * 8 + nf;
                attacks |= ((Bitboard)1 << targetSq);
            }
        }

        BB_PseudoAttacks_Knight[sq] = attacks;
    }

    // Initialize king pseudo-attack tables
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;
        Bitboard attacks = 0ULL;

        for (int i = 0; i < 8; ++i) {
            int nf = file + KingOffsets[i][0];
            int nr = rank + KingOffsets[i][1];

            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                int targetSq = nr * 8 + nf;
                attacks |= ((Bitboard)1 << targetSq);
            }
        }

        BB_PseudoAttacks_King[sq] = attacks;
    }

    // Initialize pawn attack tables
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;

        // White pawn attacks: NE (rank+1, file+1) and NW (rank+1, file-1)
        Bitboard wAtk = 0ULL;
        if (rank < 7) {
            if (file > 0) {
                int nw = (rank + 1) * 8 + (file - 1);
                wAtk |= ((Bitboard)1 << nw);
            }
            if (file < 7) {
                int ne = (rank + 1) * 8 + (file + 1);
                wAtk |= ((Bitboard)1 << ne);
            }
        }
        BB_PawnAttacks[WHITE][sq] = wAtk;

        // Black pawn attacks: SE (rank-1, file+1) and SW (rank-1, file-1)
        Bitboard bAtk = 0ULL;
        if (rank > 0) {
            if (file > 0) {
                int sw = (rank - 1) * 8 + (file - 1);
                bAtk |= ((Bitboard)1 << sw);
            }
            if (file < 7) {
                int se = (rank - 1) * 8 + (file + 1);
                bAtk |= ((Bitboard)1 << se);
            }
        }
        BB_PawnAttacks[BLACK][sq] = bAtk;
    }

    // Initialize pawn push tables
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;

        // White pawn pushes: north (+8 per rank)
        Bitboard wPushes = 0ULL;
        if (rank < 7) {
            // Single push
            wPushes |= ((Bitboard)1 << ((rank + 1) * 8 + file));
        }
        if (rank == 1) {
            // Double push from starting rank
            wPushes |= ((Bitboard)1 << ((rank + 2) * 8 + file));
        }
        BB_PawnPushes[WHITE][sq] = wPushes;

        // Black pawn pushes: south (-8 per rank)
        Bitboard bPushes = 0ULL;
        if (rank > 0) {
            // Single push
            bPushes |= ((Bitboard)1 << ((rank - 1) * 8 + file));
        }
        if (rank == 6) {
            // Double push from starting rank
            bPushes |= ((Bitboard)1 << ((rank - 2) * 8 + file));
        }
        BB_PawnPushes[BLACK][sq] = bPushes;
    }

    // Initialize bishop pseudo-attack tables (max coverage on empty board)
    // Bishop directions: (+1,+1), (-1,-1), (+1,-1), (-1,+1) in (fileDelta, rankDelta)
    static const int BishopDirs[4][2] = {{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;
        Bitboard attacks = 0ULL;

        for (int d = 0; d < 4; ++d) {
            int f = file + BishopDirs[d][0];
            int r = rank + BishopDirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                attacks |= ((Bitboard)1 << (r * 8 + f));
                f += BishopDirs[d][0];
                r += BishopDirs[d][1];
            }
        }
        BB_PseudoAttacks_Bishop[sq] = attacks;
    }

    // Initialize rook pseudo-attack tables (max coverage on empty board)
    // Rook directions: (0,+1), (0,-1), (+1,0), (-1,0) in (fileDelta, rankDelta)
    static const int RookDirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq % 8;
        int rank = sq / 8;
        Bitboard attacks = 0ULL;

        for (int d = 0; d < 4; ++d) {
            int f = file + RookDirs[d][0];
            int r = rank + RookDirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                attacks |= ((Bitboard)1 << (r * 8 + f));
                f += RookDirs[d][0];
                r += RookDirs[d][1];
            }
        }
        BB_PseudoAttacks_Rook[sq] = attacks;
    }
}

// --- Geometry table: BB_Line ---
//
// BB_Line[s1][s2] holds every square on the line segment between s1 and s2
// (rank, file or diagonal), INCLUDING s1 and s2. If s1 and s2 share no rank,
// file or diagonal the entry is empty; for s1 == s2 it is just that square.
// BB_Between (strictly between) is this line minus the endpoints, built
// separately.

static Bitboard BB_Line[64][64];

// Line family: 0=rank, 1=file, 2=diag "/" (rank-file const),
// 3=diag "\" (rank+file const).
static int bb_lineKey(int sq, int axis) {
    switch (axis) {
        case 0: return sq / 8;
        case 1: return sq % 8;
        case 2: return (sq / 8) - (sq % 8);
        default: return (sq / 8) + (sq % 8);
    }
}

// A monotonic coordinate along the line for each family.
static int bb_lineCoord(int sq, int axis) {
    switch (axis) {
        case 0: return sq % 8;            // along a rank: the file
        case 1: return sq / 8;            // along a file: the rank
        case 2: return sq / 8;            // along a "/" diagonal: the rank
        default: return sq / 8;           // along a "\" diagonal: the rank
    }
}

void bb_initLine(void) {
    for (int a = 0; a < 64; ++a) {
        for (int b = 0; b < 64; ++b) {
            if (a == b) {
                BB_Line[a][b] = bb_Square(a);
                continue;
            }
            Bitboard line = 0;
            for (int axis = 0; axis < 4; ++axis) {
                // Two distinct squares share at most one line family.
                if (bb_lineKey(a, axis) != bb_lineKey(b, axis)) continue;
                int ca = bb_lineCoord(a, axis);
                int cb = bb_lineCoord(b, axis);
                int lo = ca < cb ? ca : cb;
                int hi = ca < cb ? cb : ca;
                for (int s = 0; s < 64; ++s) {
                    if (bb_lineKey(s, axis) != bb_lineKey(a, axis)) continue;
                    int c = bb_lineCoord(s, axis);
                    if (c >= lo && c <= hi) {
                        line |= bb_Square(s);
                    }
                }
                break; // found the shared family
            }
            BB_Line[a][b] = line;
        }
    }
}

// --- Geometry table: BB_Between ---
//
// BB_Between[s1][s2] holds the squares STRICTLY between s1 and s2 on a shared
// rank, file or diagonal (the endpoints s1 and s2 are excluded). It is derived
// directly from BB_Line by removing the two endpoints, so bb_initLine() must
// run first. Empty when s1 == s2 or the two squares are not collinear.

static Bitboard BB_Between[64][64];

void bb_initBetween(void) {
    for (int a = 0; a < 64; ++a) {
        for (int b = 0; b < 64; ++b) {
            BB_Between[a][b] = BB_Line[a][b] & ~(bb_Square(a) | bb_Square(b));
        }
    }
}

// --- Geometry table: BB_RayPass ---
//
// BB_RayPass[s1][s2] holds the squares on the line beyond s2 when travelling
// from s1, i.e. the part of the full rank/file/diagonal line that lies on the
// far side of s2 in the s1 -> s2 direction. It is directional
// (RayPass[a][b] != RayPass[b][a] in general), empty for s1 == s2 or
// non-collinear pairs, and empty when s2 is the far edge of the line.

static Bitboard BB_RayPass[64][64];

void bb_initRayPass(void) {
    for (int a = 0; a < 64; ++a) {
        for (int b = 0; b < 64; ++b) {
            if (a == b) {
                BB_RayPass[a][b] = 0;
                continue;
            }
            Bitboard ray = 0;
            for (int axis = 0; axis < 4; ++axis) {
                // Two distinct squares share at most one line family.
                if (bb_lineKey(a, axis) != bb_lineKey(b, axis)) continue;
                int ca = bb_lineCoord(a, axis);
                int cb = bb_lineCoord(b, axis);
                for (int s = 0; s < 64; ++s) {
                    if (bb_lineKey(s, axis) != bb_lineKey(a, axis)) continue;
                    int c = bb_lineCoord(s, axis);
                    if (cb > ca && c > cb) ray |= bb_Square(s);
                    else if (cb < ca && c < cb) ray |= bb_Square(s);
                }
                break; // found the shared family
            }
            BB_RayPass[a][b] = ray;
        }
    }
}

// --- Move generation (bitboard) ---
//
// bb_genKnightMoves returns the bitboard of every square the knights in
// pieceBB can move to: a knight may move to any of its attack squares that is
// not occupied by a friendly piece. Captures (moves onto enemyBB) and quiet
// moves (onto empty squares) are both part of the result; the caller can
// recover the capture subset as `result & enemyBB`.
//
//   pieceBB : bitboard of the knights (the "from" squares).
//   enemyBB : bitboard of enemy pieces (marks which result squares are captures).

Bitboard bb_genKnightMoves(Bitboard pieceBB, Bitboard enemyBB) {
    (void) enemyBB;
    Bitboard froms = pieceBB;
    Bitboard result = 0;
    while (froms) {
        int from = bb_pop_lsb(&froms);
        // A knight may not land on a friendly square.
        result |= BB_PseudoAttacks_Knight[from] & ~pieceBB;
    }
    return result;
}

// bb_genKingMoves returns the bitboard of every square the king on kingSq can
// move to: any adjacent square that is not occupied by a friendly piece.
// Captures (moves onto enemyBB) and quiet moves (onto empty squares) are both
// in the result; the capture subset is `result & enemyBB`.
//
//   kingSq     : the king's current square (0..63).
//   friendlyBB : friendly pieces the king may not move onto.
//   enemyBB    : enemy pieces (marks which result squares are captures).

Bitboard bb_genKingMoves(int kingSq, Bitboard friendlyBB, Bitboard enemyBB) {
    (void) enemyBB;
    // The king may not step onto a friendly square.
    return BB_PseudoAttacks_King[kingSq] & ~friendlyBB;
}

Bitboard bb_slidingAttack_bishop(int sq, Bitboard occupied) {
    int file = sq % 8;
    int rank = sq / 8;
    Bitboard attacks = 0ULL;

    // 4 diagonal directions: NE(+1,+1), NW(-1,+1), SE(+1,-1), SW(-1,-1)
    const int dirs[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

    for (int d = 0; d < 4; d++) {
        int f = file + dirs[d][0];
        int r = rank + dirs[d][1];

        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            int target = r * 8 + f;
            attacks |= ((Bitboard)1 << target);
            if (occupied & ((Bitboard)1 << target)) break;
            f += dirs[d][0];
            r += dirs[d][1];
        }
    }
    return attacks;
}

Bitboard bb_slidingAttack_rook(int sq, Bitboard occupied) {
    int file = sq % 8;
    int rank = sq / 8;
    Bitboard attacks = 0ULL;

    // 4 orthogonal directions: N(0,+1), S(0,-1), E(+1,0), W(-1,0)
    const int dirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};

    for (int d = 0; d < 4; d++) {
        int f = file + dirs[d][0];
        int r = rank + dirs[d][1];

        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            int target = r * 8 + f;
            attacks |= ((Bitboard)1 << target);
            if (occupied & ((Bitboard)1 << target)) break;
            f += dirs[d][0];
            r += dirs[d][1];
        }
    }
    return attacks;
}

Bitboard bb_slidingAttack_queen(int sq, Bitboard occupied) {
    return bb_slidingAttack_bishop(sq, occupied) | bb_slidingAttack_rook(sq, occupied);
}

void bb_printBoard(BitboardState *s) {
    const char PieceChars[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};

    // Header with file labels
    printf("   a b c d e f g h\n");
    printf("  +---+---+---+---+---+---+---+---+\n");

    for (int rank = 7; rank >= 0; rank--) {
        printf("%d |", rank + 1);
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            Bitboard sqBB = bb_Square(sq);

            char ch = '.';
            bool found = false;

            // Check en passant first
            if (sq == s->EnPassant) {
                ch = '+';
                found = true;
            }

            // Check all piece bitboards
            for (int color = 0; color < 2 && !found; color++) {
                for (int pt = 0; pt < 6; pt++) {
                    if (s->Pieces[color][pt] & sqBB) {
                        ch = PieceChars[pt];
                        if (color == BLACK) {
                            ch = ch - 'A' + 'a';
                        }
                        found = true;
                        break;
                    }
                }
            }

            printf(" %c |", ch);
        }
        printf("\n");
        printf("  +---+---+---+---+---+---+---+---+\n");
    }

    printf("   a b c d e f g h\n");

    // Metadata line
    printf("Active: %s, Castling: ", s->ActiveColor == WHITE ? "White" : "Black");
    if (s->Castling == 0) {
        printf("-");
    } else {
        if (bb_hasCastleRight(s, BB_CASTLE_WK)) printf("K");
        if (bb_hasCastleRight(s, BB_CASTLE_WQ)) printf("Q");
        if (bb_hasCastleRight(s, BB_CASTLE_BK)) printf("k");
        if (bb_hasCastleRight(s, BB_CASTLE_BQ)) printf("q");
    }
    printf(", EP: %s", s->EnPassant >= 0 ? "yes" : "no");
    printf(", HM: %u, FM: %u\n", s->HalfmoveClock, s->FullmoveNumber);
}

// --- Magic bitboard tables for bishop attacks ---
//
// A bishop's relevant occupancy is its full diagonal ray coverage. For each
// square a 64-bit "magic" number maps every relevant occupancy to an index via
//   index = (relOcc * magic) >> (64 - k)
// where k is the (fixed) number of relevant bits; the index then indexes a
// per-square table holding the correct attack bitboard. The max k for a bishop
// is 13, so each table has 1 << 13 = 8192 entries.
//
// The magic numbers are found once offline by the "few-bits" search (Tord
// Romstad): try sparse candidates (bitwise AND of several randoms) and keep the
// first for which the index is a function of the attack. They are baked here so
// the table is built deterministically at init with no runtime search. Every
// square has a non-zero magic (a bishop always has at least one diagonal).

#define BB_BISHOP_MAGIC_TABLE_SIZE 8192

static const Bitboard BB_MagicBishop_init[64] = {
    0x4004042822042204ULL, 0x2000880104002002ULL, 0x040c002402100001ULL,
    0x0838012830000000ULL, 0x8004010800001202ULL, 0x00120201a0010401ULL,
    0x4000141004100000ULL, 0x0080c64128044000ULL, 0x0208102044410044ULL,
    0x0000013400820006ULL, 0x00001400802c0430ULL, 0x000000c040400000ULL,
    0x8000140414010004ULL, 0x0001020010050800ULL, 0x0000209801040200ULL,
    0x8a10200841041000ULL, 0x4a02000490204800ULL, 0x0000224450010048ULL,
    0x000080040444200aULL, 0x800020805a000100ULL, 0x4400104101008000ULL,
    0x0081001044000900ULL, 0x0000102008001110ULL, 0x0012000021012800ULL,
    0xc001081040024340ULL, 0x000c008004060080ULL, 0x8000080410024a40ULL,
    0x000005021a710200ULL, 0x0801100240440400ULL, 0x0002001804040080ULL,
    0x8004012828405a00ULL, 0x0000808000082800ULL, 0x0010042102040080ULL,
    0x0000428201006a40ULL, 0x0284000a21040400ULL, 0x4400008003008200ULL,
    0x0030000200010101ULL, 0x900108002204d000ULL, 0x0002050010002200ULL,
    0x2404340010534140ULL, 0x0000210410306000ULL, 0x0000080040a00802ULL,
    0x4600860800100200ULL, 0x4018208810400400ULL, 0x4124018210010040ULL,
    0x1210600060808300ULL, 0x0004008019001010ULL, 0x8004040724080650ULL,
    0x0002804104400200ULL, 0x5200090441002000ULL, 0x0080450408004400ULL,
    0x2400000622021000ULL, 0x8001821c00408400ULL, 0x4488602002000820ULL,
    0x0040010012008100ULL, 0x0010022841002004ULL, 0x200050420c442000ULL,
    0x800c001088004800ULL, 0x0420000009004800ULL, 0x02d0080000485020ULL,
    0xc004000202020202ULL, 0xc000080819818200ULL, 0x1408024730016600ULL,
    0x000490420a040010ULL,
};

static Bitboard BB_MagicBishop[64];
static Bitboard BB_RelativeOcc_Bishop[64];
static Bitboard BB_BishopAttacks[64][BB_BISHOP_MAGIC_TABLE_SIZE];

static void bb_initMagics_bishop(void) {
    for (int sq = 0; sq < 64; ++sq) {
        // The relevant occupancy is the full ray coverage; copy the baked magic.
        BB_MagicBishop[sq] = BB_MagicBishop_init[sq];
        Bitboard relOcc = BB_PseudoAttacks_Bishop[sq];
        BB_RelativeOcc_Bishop[sq] = relOcc;

        int k = bb_popcount(relOcc);

        // No diagonal squares: only the empty configuration exists, index 0.
        if (k == 0) {
            BB_BishopAttacks[sq][0] = bb_slidingAttack_bishop(sq, 0ULL);
            continue;
        }

        // Fill the table by enumerating every relevant occupancy (submask of
        // relOcc). A valid magic guarantees every occupancy of the same attack
        // lands in the same entry, so a later write can never clobber a
        // different attack.
        int shift = 64 - k;
        Bitboard sub = relOcc;
        do {
            unsigned int index = (unsigned int)((sub * BB_MagicBishop[sq]) >> shift);
            BB_BishopAttacks[sq][index] = bb_slidingAttack_bishop(sq, sub);
            if (sub == 0) break;
            sub = (sub - 1) & relOcc;
        } while (1);
    }
}

Bitboard bb_bishopAttacks(int sq, Bitboard occupied) {
    Bitboard relOcc = occupied & BB_RelativeOcc_Bishop[sq];

    // k is the (fixed) number of relevant bits for the square, NOT the popcount
    // of the masked occupancy: the index must use the same k the table was built
    // with. k == 0 would shift by 64 (undefined behaviour), so the empty
    // configuration always maps to index 0.
    int k = bb_popcount(BB_RelativeOcc_Bishop[sq]);
    if (k == 0) {
        return BB_BishopAttacks[sq][0];
    }

    unsigned int index = (unsigned int)((relOcc * BB_MagicBishop[sq]) >> (64 - k));
    return BB_BishopAttacks[sq][index];
}

// --- Magic bitboard tables for rook attacks ---
//
// A rook's relevant occupancy is its full rank/file ray coverage, which is 14
// bits for every square (7 squares on the rank + 7 on the file), so each
// per-square table has 1 << 14 = 16384 entries. The magic numbers are found once
// offline by the "few-bits" search (Tord Romstad) and baked here; every square
// has a non-zero magic.

#define BB_ROOK_MAGIC_TABLE_SIZE 16384

static const Bitboard BB_MagicRook_init[64] = {
    0x2401801800148000ULL, 0x00200020480a4d00ULL, 0x4200021120054200ULL,
    0x1000410010002080ULL, 0x0200014042100a00ULL, 0x0020004481001120ULL,
    0x0380082201408100ULL, 0x400a128000204100ULL, 0x00013200d0941000ULL,
    0x8000040024b10200ULL, 0x4040020006010400ULL, 0x0000100008820300ULL,
    0x20000200c4080100ULL, 0x0000020020840040ULL, 0x4002000051084024ULL,
    0x0000080880254100ULL, 0x0480040100214000ULL, 0x00200a0002001c80ULL,
    0x00826060001c0100ULL, 0x800034040000b000ULL, 0x0000050020020100ULL,
    0x6004020001008048ULL, 0x4000040008a21108ULL, 0x90000048a104000cULL,
    0x2102000400204400ULL, 0x0000042400890800ULL, 0x1c24120002012000ULL,
    0x0000204104000200ULL, 0x00408f0200004600ULL, 0x1021008002001040ULL,
    0x2400002008014200ULL, 0x00000100c4080012ULL, 0x0000042800823000ULL,
    0x0000020082004000ULL, 0x0000200a01000300ULL, 0x8018100008004080ULL,
    0x0000068010002040ULL, 0x4000144081040002ULL, 0x0400002000844600ULL,
    0x0800600851800040ULL, 0x0000802001000b00ULL, 0x0000048800100400ULL,
    0x00000a100900b000ULL, 0x0000400800880020ULL, 0x8840024480090800ULL,
    0x1880040020010410ULL, 0x4000020140440008ULL, 0x00000020000886e0ULL,
    0x0002004081001200ULL, 0x0082100208220200ULL, 0x100a640200010600ULL,
    0x4100024449008100ULL, 0x5000004d08840420ULL, 0x8400004024008100ULL,
    0x00022401004c8200ULL, 0x0200003950021080ULL, 0x0000048010628442ULL,
    0x000070203a000802ULL, 0x002001044062000aULL, 0x20000301000b9007ULL,
    0x0000020004c88506ULL, 0x2000040040a18812ULL, 0x403308020240806cULL,
    0x0000000841201082ULL,
};

static Bitboard BB_MagicRook[64];
static Bitboard BB_RelativeOcc_Rook[64];
static Bitboard BB_RookAttacks[64][BB_ROOK_MAGIC_TABLE_SIZE];

static void bb_initMagics_rook(void) {
    for (int sq = 0; sq < 64; ++sq) {
        // The relevant occupancy is the full rank/file ray coverage; copy the
        // baked magic.
        BB_MagicRook[sq] = BB_MagicRook_init[sq];
        Bitboard relOcc = BB_PseudoAttacks_Rook[sq];
        BB_RelativeOcc_Rook[sq] = relOcc;

        int k = bb_popcount(relOcc);

        // No rank/file squares: only the empty configuration exists, index 0.
        if (k == 0) {
            BB_RookAttacks[sq][0] = bb_slidingAttack_rook(sq, 0ULL);
            continue;
        }

        // Fill the table by enumerating every relevant occupancy (submask of
        // relOcc). A valid magic guarantees every occupancy of the same attack
        // lands in the same entry, so a later write can never clobber a
        // different attack.
        int shift = 64 - k;
        Bitboard sub = relOcc;
        do {
            unsigned int index = (unsigned int)((sub * BB_MagicRook[sq]) >> shift);
            BB_RookAttacks[sq][index] = bb_slidingAttack_rook(sq, sub);
            if (sub == 0) break;
            sub = (sub - 1) & relOcc;
        } while (1);
    }
}

Bitboard bb_rookAttacks(int sq, Bitboard occupied) {
    Bitboard relOcc = occupied & BB_RelativeOcc_Rook[sq];

    // k is the (fixed) number of relevant bits for the square, NOT the popcount
    // of the masked occupancy: the index must use the same k the table was built
    // with. k == 0 would shift by 64 (undefined behaviour), so the empty
    // configuration always maps to index 0.
    int k = bb_popcount(BB_RelativeOcc_Rook[sq]);
    if (k == 0) {
        return BB_RookAttacks[sq][0];
    }

    unsigned int index = (unsigned int)((relOcc * BB_MagicRook[sq]) >> (64 - k));
    return BB_RookAttacks[sq][index];
}

// Single entry point that initialises every magic bitboard lookup table. Mirrors
// Stockfish's do_magics(); call this once at startup.
void bb_initMagics(void) {
    bb_initMagics_bishop();
    bb_initMagics_rook();
}
