#include <assert.h>
#include "base.h"
#include "chess.h"
#include "chess.c"
#include "engine.c"

static bool test_findBestSequence() {
    bool Success = true;
    // The engine must make the obvious best moves.
    // Take the free piece
    BoardState BS = ch_parseFEN("r1bqkbnr/pppppppp/8/8/3n4/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    SequenceList SeqL = en_findBestSequence(&BS);
    Success &= SeqL.List[0].Moves[0].From.Rank == 2;
    Success &= SeqL.List[0].Moves[0].From.File == 5;
    Success &= SeqL.List[0].Moves[0].To.Rank == 3;
    Success &= SeqL.List[0].Moves[0].To.File == 3;

    BS = ch_parseFEN("rnb1kb1r/pp3ppp/8/q2np3/8/2NBBP2/PP4PP/R2QK1NR w KQkq - 0 11");
    SeqL = en_findBestSequence(&BS);
    Success &= SeqL.List[0].Moves[0].From.Rank == 2;
    Success &= SeqL.List[0].Moves[0].From.File == 3;
    Success &= SeqL.List[0].Moves[0].To.Rank == 4;
    Success &= SeqL.List[0].Moves[0].To.File == 1;

    BS = ch_parseFEN("r1b2rk1/1pp1bppp/p3p3/3q4/1n1Pn3/2N2NP1/PP1QPPBP/R1BR2K1 b - - 10 12");
    SeqL = en_findBestSequence(&BS);
    Success &= SeqL.List[0].Moves[0].From.Rank == 3;
    Success &= SeqL.List[0].Moves[0].From.File == 4;
    Success &= SeqL.List[0].Moves[0].To.Rank == 1;
    Success &= SeqL.List[0].Moves[0].To.File == 3;

    // Mate in 2
    BS = ch_parseFEN("6R1/4r2p/5ppk/1b6/1B3p2/1BP5/PP4PP/6K1 b - - 3 34");
    SeqL = en_findBestSequence(&BS);
    Success &= SeqL.List[0].Moves[0].From.Rank == 6;
    Success &= SeqL.List[0].Moves[0].From.File == 4;
    Success &= SeqL.List[0].Moves[0].To.Rank == 0;
    Success &= SeqL.List[0].Moves[0].To.File == 4;

    return Success;
}

int main() {
    bool Success = true;

    Success &= test_findBestSequence();
    assert(Success);

    return Success;
}
