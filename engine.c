#ifndef _ENGINE_C_
#define _ENGINE_C_

// Half moves.
#define ENGINE_DEPTH 6

#define INFINITY (__builtin_inff())

#define ABS(x) (((x) < 0.0f) ? -(x) : (x))

typedef struct {
    int Depth;
    Move Moves[ENGINE_DEPTH]; // White and Black
    float Eval;
} Sequence;

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
            if (!P) {
                continue;
            }
            float PieceVal = 0.0f;
            switch (P) {
                case 'P': {
                    PieceVal = PAWN;
                    break;
                }
                case 'N': {
                    PieceVal = KNIGHT;
                    break;
                }
                case 'B': {
                    PieceVal = BISHOP;
                    break;
                }
                case 'R': {
                    PieceVal = ROOK;
                    break;
                }
                case 'Q': {
                    PieceVal = QUEEN;
                    break;
                }
                case 'p': {
                    PieceVal = -PAWN;
                    break;
                }
                case 'n': {
                    PieceVal = -KNIGHT;
                    break;
                }
                case 'b': {
                    PieceVal = -BISHOP;
                    break;
                }
                case 'r': {
                    PieceVal = -ROOK;
                    break;
                }
                case 'q': {
                    PieceVal = -QUEEN;
                    break;
                }
                default: {
                    // always two kings, no point.
                    break;
                }
            }
            float HalfBoard = (float)BOARDSIZE / 2.0f;
            float PiecePosVal = (HalfBoard - ABS((float)R - HalfBoard)) +
                                (HalfBoard - ABS((float)F - HalfBoard));
            PiecePosVal /= 8.0f;
            Eval += PiecePosVal;
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

Sequence en_recurrentEvaluateMove(BoardState *BS, Sequence Seq, int Depth, float Alpha, float Beta) {
    if (Depth == ENGINE_DEPTH) {
        Seq.Depth = Depth;
        Seq.Eval = en_evaluatePosition(BS);
        return Seq;
    }
    bool BlackToMove = BS->BlackToMove;

    Sequence CurrBestSeq = Seq;
    CurrBestSeq.Eval = BlackToMove ? INFINITY : -INFINITY;
    CurrBestSeq.Depth = Depth;
    MoveList LegalMoves = ch_getLegalMoves(BS);
    if (LegalMoves.Count == 0) {
        if (!ch_inCheck(BS)) {
            CurrBestSeq.Eval = 0.0f;
            return CurrBestSeq;
        }
    }
    for (int I = 0; I < LegalMoves.Count; ++I) {
        Move Mv = LegalMoves.List[I];

        BoardState BSCopy = *BS;
        ch_applyMove(&BSCopy, Mv);
        Seq.Moves[Depth] = Mv;
        Sequence CurrSeq = en_recurrentEvaluateMove(&BSCopy, Seq, Depth + 1, Alpha, Beta);

        if (BlackToMove) {
            if (CurrSeq.Eval < CurrBestSeq.Eval) {
                CurrBestSeq = CurrSeq;
            } else if (CurrSeq.Eval == INFINITY && CurrBestSeq.Eval == INFINITY) {
                if (CurrSeq.Depth > CurrBestSeq.Depth) {
                    CurrBestSeq = CurrSeq;
                }
            } else if (CurrSeq.Eval == -INFINITY && CurrBestSeq.Eval == -INFINITY) {
                if (CurrSeq.Depth < CurrBestSeq.Depth) {
                    CurrBestSeq = CurrSeq;
                }
            }
            if (CurrBestSeq.Eval < Beta) {
                Beta = CurrBestSeq.Eval;
            }
        } else {
            if (CurrSeq.Eval > CurrBestSeq.Eval) {
                CurrBestSeq = CurrSeq;
            } else if (CurrSeq.Eval == -INFINITY && CurrBestSeq.Eval == -INFINITY) {
                if (CurrSeq.Depth > CurrBestSeq.Depth) {
                    CurrBestSeq = CurrSeq;
                }
            } else if (CurrSeq.Eval == INFINITY && CurrBestSeq.Eval == INFINITY) {
                if (CurrSeq.Depth < CurrBestSeq.Depth) {
                    CurrBestSeq = CurrSeq;
                }
            }
            if (CurrBestSeq.Eval > Alpha) {
                Alpha = CurrBestSeq.Eval;
            }
        }
        if (Alpha > Beta) {
            break;
        }
    }
    return CurrBestSeq;
}

Sequence en_findBestSequence(BoardState *BS) {
    Sequence Seq = en_recurrentEvaluateMove(BS, (Sequence){0}, 0,
            -INFINITY, INFINITY);
    return Seq;
}

#endif
