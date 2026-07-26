#include <assert.h>
#include "base.h"
#include "chess.h"
#include "chess.c"

#include <stdio.h>

static const char *STARTING_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static BoardState g_BoardState = {0};

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

int main() {
    g_BoardState = ch_parseFEN(STARTING_POSITION_FEN);
    printBoardState(&g_BoardState);
    return 0;
}
