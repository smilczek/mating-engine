#ifndef _ENGINE_C_
#define _ENGINE_C_

// Half moves.
#define DEPTH 4

#define INFINITY (__builtin_inff())

typedef struct {
    Move Moves[DEPTH]; // White and Black
    float Eval;
} Sequence;

typedef struct {
    Sequence List[1];
} SequenceList;

float en_evaluatePosition(BoardState *BS) {
    // TODO(smilczek): Test if counting the legal moves for
    //                 each side is also a valid eval strat.
    //                 (The diff MyMoves - TheirMoves)
    float PAWN   = 1.0f;
    float KNIGHT = 3.0f;
    float BISHOP = 3.0f;
    float ROOK   = 5.0f;
    float QUEEN  = 9.0f;
    float Eval = 0.0f;
    for (int R = 0; R < BOARDSIZE; ++R) {
        for (int F = 0; F < BOARDSIZE; ++F) {
            char P = ch_pieceAt(BS, R, F);
            float BlackMul = ch_isBlackPiece(P) ? -1.0f : 1.0f;
            float PieceVal = 0.0f;
            switch (lowercase(P)) {
                case 'p': {
                    PieceVal = PAWN;
                    break;
                }
                case 'n': {
                    PieceVal = KNIGHT;
                    break;
                }
                case 'b': {
                    PieceVal = BISHOP;
                    break;
                }
                case 'r': {
                    PieceVal = ROOK;
                    break;
                }
                case 'q': {
                    PieceVal = QUEEN;
                    break;
                }
                default: {
                    // always two kings, no point.
                    break;
                }
            }
            PieceVal *= BlackMul;
            Eval += PieceVal;
        }
    }
    // BoardState BSCopy = *BS;
    // BSCopy.BlackToMove = false;
    // MoveList WhiteMoves = ch_generatePseudoLegalMoves(&BSCopy);
    // BSCopy.BlackToMove = true;
    // MoveList BlackMoves = ch_generatePseudoLegalMoves(&BSCopy);
    // return (float)(WhiteMoves.Count - BlackMoves.Count);
    return Eval;
}

Sequence en_recurrentEvaluateMove(BoardState *BS, Sequence Seq, int Depth) {
    if (Depth == DEPTH) {
        Seq.Eval = en_evaluatePosition(BS);
        return Seq;
    }
    bool BlackToMove = BS->BlackToMove;

    Sequence CurrBestSeq = Seq;
    CurrBestSeq.Eval = BlackToMove ? INFINITY : -INFINITY;
    MoveList LegalMoves = ch_getLegalMoves(BS);
    for (int I = 0; I < LegalMoves.Count; ++I) {
        Move Mv = LegalMoves.List[I];

        BoardState BSCopy = *BS;
        ch_applyMove(&BSCopy, Mv);
        Seq.Moves[Depth] = Mv;
        Sequence CurrSeq = en_recurrentEvaluateMove(&BSCopy, Seq, Depth + 1);

        if (BlackToMove && CurrSeq.Eval < CurrBestSeq.Eval ||
                !BlackToMove && CurrSeq.Eval > CurrBestSeq.Eval) {
            CurrBestSeq = CurrSeq;
        }
    }
    return CurrBestSeq;
}

SequenceList en_findBestSequence(BoardState *BS) {
    SequenceList SeqList = {0};
    SeqList.List[0] = en_recurrentEvaluateMove(BS, (Sequence){0}, 0);
    return SeqList;
}

#endif
