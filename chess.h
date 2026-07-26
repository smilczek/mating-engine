#ifndef _CHESS_H_
#define _CHESS_H_

#define BOARDSIZE 8

typedef struct {
    int Rank, File;
} Coord;

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

#endif
