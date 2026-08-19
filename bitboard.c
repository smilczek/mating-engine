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
