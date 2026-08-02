#include "base.h"

#include <assert.h>

#include "chess.h"
#include "chess.c"
#include "engine.c"

#include <stdio.h>

static const char *STARTING_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static void printMove(BoardState *BS, Move Mv) {
    char P = uppercase(ch_pieceAtCoord(BS, Mv.From));
    if (P != 'P') {
        printf("%c", P);
    }
    if (ch_pieceAtCoord(BS, Mv.To)) {
        if (P == 'P') {
            printf("%c", ch_fileToChar(Mv.From.File));
        }
        printf("x");
    }

    printf("%c%c", ch_fileToChar(Mv.To.File), ch_rankToChar(Mv.To.Rank));
}
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
    BoardState BS = ch_parseFEN(STARTING_POSITION_FEN);
    printf("%d. ", 1);
    for (int i = 0; i < 100; ++i) {
        Sequence Seq = en_findBestSequence(&BS);
        printMove(&BS, Seq.Moves[0]);
        printf(" ");
        ch_applyMove(&BS, Seq.Moves[0]);
        if (i % 2 == 1) {
            printf("\n%d. ", i / 2 + 2);
        }
    }
    printf("\n");
    printBoardState(&BS);

    return 0;
}
