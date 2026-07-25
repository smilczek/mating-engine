#include "base.h"
#include "chess.h"
#include "chess.c"

#include <stdio.h>

static BoardState g_BoardState = {0};

static void printBoardState(BoardState *BS) {
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
