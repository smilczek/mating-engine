#include <assert.h>
#include "base.h"
#include "chess.h"
#include "chess.c"
#include "engine.c"

#include <stdio.h>

static const char *STARTING_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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

static void printMove(BoardState *BS, Move Mv) {
    char P = ch_pieceAtCoord(BS, Mv.From);
    if (lowercase(P) != 'p') {
        printf("%c", P);
    }
    printf("%c%c", ch_fileToChar(Mv.To.File), ch_rankToChar(Mv.To.Rank));
}

int main() {
    // BoardState BS = ch_parseFEN("6R1/4r2p/5ppk/1b6/1B3p2/1BP5/PP4PP/6K1 b - - 3 34");
    BoardState BS = ch_parseFEN(STARTING_POSITION_FEN);
    Sequence Seq = en_findBestSequence(&BS);
    for (int i = 0; i < arr_len(Seq.Moves); i += 2) {
        printf("%d. ", (i + 2 / 2));
        printMove(&BS, Seq.Moves[i]);
        printf(" ");
        printMove(&BS, Seq.Moves[i + 1]);
        printf(" ");
    }
    printf("\n");
    return 0;
}
