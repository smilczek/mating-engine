#ifndef _CHESS_H_
#define _CHESS_H_

#define BOARDSIZE 8

#define MOVE_LIST_SIZE 256

typedef struct {
    int Rank, File;
} Coord;

typedef struct {
    int R, F; // Rank, File
} Dir;

typedef struct {
    char Board[BOARDSIZE][BOARDSIZE];

    bool BlackToMove;

    // Castling rights
    bool CR_WK; // White King
    bool CR_WQ; // White Queen
    bool CR_BK; // Black King
    bool CR_BQ; // Black Queen

    // En passant target
    char EnPassant[2]; // e.g. e3

    // 50-move rule
    int HalfmoveClock;

    // Starts at 0. (Normally starts at 1, stupid,
    // I'd rather keep ZII, add 1 for display)
    // Increments after black's move.
    int FullmoveNumber;
} BoardState;

typedef struct {
    Coord From;
    Coord To;
    char Promotion;
} Move;

typedef struct {
    Move List[MOVE_LIST_SIZE];
    int Count;
} MoveList;

static const int BISHOP_DIR_SET[4][2] = {
    { 1,  1},
    { 1, -1},
    {-1, -1},
    {-1,  1}
};

static const int ROOK_DIR_SET[4][2] = {
    { 0,  1},
    { 0, -1},
    { 1,  0},
    {-1,  0}
};

static const int KNIGHT_MOVE_SET[8][2] = {
    { 2,  1},
    { 2, -1},
    {-2,  1},
    {-2, -1},
    { 1,  2},
    {-1,  2},
    { 1, -2},
    {-1, -2}
};

#endif
