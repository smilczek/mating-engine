#include <assert.h>
#include "base.h"
#include "chess.h"
#include "chess.c"
#include "engine.c"

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

static void printMove(BoardState *BS, Move Mv) {
    char P = ch_pieceAtCoord(BS, Mv.From);
    if (lowercase(P) != 'p') {
        printf("%c", P);
    }
    printf("%c%c\n", ch_fileToChar(Mv.To.File), ch_rankToChar(Mv.To.Rank));
}

int main() {
    BoardState BS = ch_parseFEN("rnbqkbnr/pppp1ppp/8/4p2Q/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 1 2");
    SequenceList SeqL = en_findBestSequence(&BS);
    printMove(&BS, SeqL.List[0].Moves[0]);
    return 0;
}
