#include <assert.h>

#define BOARDSIZE 8

#define bool char
#define false 0
#define true 1

typedef struct {
    int Turn;
    bool BlackToPlay;
    char Board[BOARDSIZE * BOARDSIZE];

    // Castling rights
    bool CR_WK; // White King
    bool CR_WQ; // White Queen
    bool CR_BK; // Black King
    bool CR_BQ; // Black Queen

    // En passant target
    char EnPassant[2]; // e.g. e3
} BoardState;

typedef struct {
    int Rank, File;
} Coord;


static char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

// The board shall be so
// F,R
// 0,0 is A1,
// 0,7 is A8,
// 7,7 is H8.
// upper case means white
// lower case means black
static void initBoardState(BoardState *BS) {
    const char *BackrankSetup = "RNBQKBNR";
    const char *PawnSetup =     "PPPPPPPP";
    // TODO(smilczek) verify BackrankSetup is of BOARDSIZE length

    BS->Turn = 0;
    BS->BlackToPlay = 0;

    BS->CR_WK = true;
    BS->CR_WQ = true;
    BS->CR_BK = true;
    BS->CR_BQ = true;

    BS->EnPassant[0] = '\0';
    BS->EnPassant[1] = '\0';

    int Rank = 0;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = BackrankSetup[File];
    }
    Rank = 1;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = PawnSetup[File];
    }
    Rank = 7;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = lowercase(BackrankSetup[File]);
    }
    Rank = 6;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = lowercase(PawnSetup[File]);
    }
}

// Parse coord like e4, b2 etc.
// 2 chars always.
// lowercase assumed.
// no verification.
static Coord parseCoordinateStr(char *CoordStr) {
    assert(CoordStr[0] >= 'a' && CoordStr[0] <= 'h');
    assert(CoordStr[1] >= '1' && CoordStr[1] <= '8');

    Coord Ret = {0};
    Ret.Rank = CoordStr[0] - 'a';
    Ret.File = CoordStr[1] - '1';
    return Ret;
}
