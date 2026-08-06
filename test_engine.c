#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "base.h"
#include "chess.h"
#include "chess.c"
#include "engine.c"

#include <stdio.h>

static bool test_findBestSequence() {
    bool Success = true;
    // The engine must make the obvious best moves.
    // Take the free piece
    BoardState BS = ch_parseFEN("r1bqkbnr/pppppppp/8/8/3n4/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    Sequence Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 2;
    Success &= Seq.Moves[0].From.File == 5;
    Success &= Seq.Moves[0].To.Rank == 3;
    Success &= Seq.Moves[0].To.File == 3;

    BS = ch_parseFEN("rnb1kb1r/pp3ppp/8/q2np3/8/2NBBP2/PP4PP/R2QK1NR w KQkq - 0 11");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 2;
    Success &= Seq.Moves[0].From.File == 3;
    Success &= Seq.Moves[0].To.Rank == 4;
    Success &= Seq.Moves[0].To.File == 1;

    BS = ch_parseFEN("r1b2rk1/1pp1bppp/p3p3/3q4/1n1Pn3/2N2NP1/PP1QPPBP/R1BR2K1 b - - 10 12");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 3;
    Success &= Seq.Moves[0].From.File == 4;
    Success &= Seq.Moves[0].To.Rank == 1;
    Success &= Seq.Moves[0].To.File == 3;

    // Deflect king, capture queen
    BS = ch_parseFEN("3Q4/8/5qp1/5pkp/8/7P/6P1/7K w - - 4 41");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 2;
    Success &= Seq.Moves[0].From.File == 7;
    Success &= Seq.Moves[0].To.Rank == 3;
    Success &= Seq.Moves[0].To.File == 7;

    // Mate in 2
    BS = ch_parseFEN("6R1/4r2p/5ppk/1b6/1B3p2/1BP5/PP4PP/6K1 b - - 3 34");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 6;
    Success &= Seq.Moves[0].From.File == 4;
    Success &= Seq.Moves[0].To.Rank == 0;
    Success &= Seq.Moves[0].To.File == 4;

    // Mate in 1
    BS = ch_parseFEN("8/6pk/4Q2p/P1q1PK1P/5PP1/8/8/8 b - - 0 50");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 4;
    Success &= Seq.Moves[0].From.File == 2;
    Success &= Seq.Moves[0].To.Rank == 1;
    Success &= Seq.Moves[0].To.File == 2;

    BS = ch_parseFEN("7r/1R5p/1N1k2p1/8/1PPN1PP1/2K5/7P/8 w - - 0 46");
    Seq = en_findBestSequence(&BS);
    Success &= (Seq.Moves[0].From.Rank == 3 &&
                Seq.Moves[0].From.File == 2 &&
                Seq.Moves[0].To.Rank == 4 &&
                Seq.Moves[0].To.File == 2) ||
               (Seq.Moves[0].From.Rank == 6 &&
                Seq.Moves[0].From.File == 1 &&
                Seq.Moves[0].To.Rank == 6 &&
                Seq.Moves[0].To.File == 3);

    // Promote pawn
    BS = ch_parseFEN("8/k7/3P4/8/8/8/5K2/8 w - - 0 1");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 5;
    Success &= Seq.Moves[0].From.File == 3;
    Success &= Seq.Moves[0].To.Rank == 6;
    Success &= Seq.Moves[0].To.File == 3;
    Success &= lowercase(Seq.Moves[2].Promotion) == 'q';

    // Drawn pawn position
    BS = ch_parseFEN("8/3k4/3P4/3K4/8/8/8/8 b - - 0 1");
    Seq = en_findBestSequence(&BS);
    Success &= Seq.Moves[0].From.Rank == 6;
    Success &= Seq.Moves[0].From.File == 3;
    Success &= Seq.Moves[0].To.Rank == 7;
    Success &= Seq.Moves[0].To.File == 3;

    return Success;
}

int main() {
    srand((unsigned int)time(NULL));
    bool Success = true;

    Success &= test_findBestSequence();
    assert(Success);

    if (Success) {
        printf("Test engine success\n");
    } else {
        printf("Test engine fail!\n");
    }

    return Success;
}
