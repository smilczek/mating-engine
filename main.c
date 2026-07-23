#include <stdio.h>

#define BOARDSIZE 8

#define bool char
#define false 0
#define true 1

typedef struct {
    int Turn;
    bool BlackToPlay;
    char Board[BOARDSIZE * BOARDSIZE];
} BoardState;

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

int main() {
    initBoardState(&g_BoardState);
    printBoardState(&g_BoardState);
    return 0;
}
