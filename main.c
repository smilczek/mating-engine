#include <stdio.h>
#include <assert.h>

#define BOARDSIZE 8

#define bool char
#define false 0
#define true 1

typedef struct {
    int Turn;
    bool BlackToPlay;
    char Board[BOARDSIZE * BOARDSIZE];
} BoardState;

typedef struct {
    int Rank, File;
} Coord;


static BoardState g_BoardState = {0};

char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

// The board shall be so
// 0,0 is A1,
// 0,7 is A8,
// 7,7 is H8.
// upper case means white
// lower case means black
void initBoardState(BoardState *BS) {
    const char *BackrankSetup = "RNBQKBNR";
    const char *PawnSetup =     "PPPPPPPP";
    // TODO(smilczek) verify BackrankSetup is of BOARDSIZE length

    BS->Turn = 0;
    BS->BlackToPlay = 0;

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

void printBoardState(BoardState *BS) {
    for (int Rank = BOARDSIZE - 1; Rank > -1; --Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            char Square = BS->Board[Rank * BOARDSIZE + File];
            if (Square == '\0') {
                Square = '.';
            }
            printf("%c", Square);
        }
        printf("\n");
    }
}

// Parse coord like e4, b2 etc.
// 2 chars always.
// lowercase assumed.
// no verification.
Coord parseCoordinateStr(char *CoordStr) {
    assert(CoordStr[0] >= 'a' && CoordStr[0] <= 'h');
    assert(CoordStr[1] >= '1' && CoordStr[1] <= '8');

    Coord Ret = {0};
    Ret.Rank = CoordStr[0] - 'a';
    Ret.File = CoordStr[1] - '1';
    return Ret;
}

void test_ParseCoordinateStr() {
    Coord Co = parseCoordinateStr("a1");
    assert(Co.Rank == 0);
    assert(Co.File == 0);

    Co = parseCoordinateStr("h8");
    assert(Co.Rank == 7);
    assert(Co.File == 7);

    Co = parseCoordinateStr("e4");
    assert(Co.Rank == 4);
    assert(Co.File == 3);

    Co = parseCoordinateStr("h5");
    assert(Co.Rank == 7);
    assert(Co.File == 4);
}

int main() {
    initBoardState(&g_BoardState);
    printBoardState(&g_BoardState);
    test_ParseCoordinateStr();
    return 0;
}
