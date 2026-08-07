#ifndef _ENGINE_C_
#define _ENGINE_C_

// Half moves.
#define MAX_DEPTH 16

#ifdef _MSC_VER
#include <math.h>
#else
#define INFINITY (__builtin_inff())
#endif

#define ABS(x) (((x) < 0.0f) ? -(x) : (x))

typedef struct {
    int Depth;
    Move Moves[MAX_DEPTH]; // White and Black
    float Eval;
} Sequence;

float en_getPieceVal(char P) {
    float PAWN   = 1.0f;
    float KNIGHT = 3.0f;
    float BISHOP = 3.0f;
    float ROOK   = 5.0f;
    float QUEEN  = 9.0f;

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
            break;
        }
    }
    return ch_isBlackPiece(P) ? -PieceVal : PieceVal;
}
float en_getPawnAdvanceVal(char P, char R) {
    float Divisor = 8.0f;
    if (P == 'P') return ((float)(R - 1) / Divisor);
    if (P == 'p') return -((float)(6 - R) / Divisor);
    return 0.0f;
}

float en_evaluatePosition(BoardState *BS) {
    float Eval = 0.0f;
    float TotalBlackPosVal = 0.0f;
    float NumOfBlack = (float)ch_getNumBlackPieces(BS);
    float TotalWhitePosVal = 0.0f;
    float NumOfWhite = (float)ch_getNumWhitePieces(BS);
    for (int R = 0; R < BOARDSIZE; ++R) {
        for (int F = 0; F < BOARDSIZE; ++F) {
            char P = ch_pieceAt(BS, R, F);
            if (!P) {
                continue;
            }
            float PieceVal = en_getPieceVal(P);
            PieceVal += en_getPawnAdvanceVal(P, R);

            float HalfBoard = (float)(BOARDSIZE - 1) / 2.0f;
            float RankPosVal = (HalfBoard - ABS((float)R - HalfBoard));
            float FilePosVal = (HalfBoard - ABS((float)F - HalfBoard));
            float PiecePosVal = RankPosVal + FilePosVal;
            PiecePosVal *= PiecePosVal;
            PiecePosVal /= 4.0f;
            if (ch_isBlackPiece(P)) {
                TotalBlackPosVal -= PiecePosVal;
            } else {
                TotalWhitePosVal += PiecePosVal;
            }

            Eval += PieceVal;
        }
    }
    Eval += (TotalBlackPosVal / NumOfBlack +
            TotalWhitePosVal / NumOfWhite);

    // Introduce some randomness
    float var = -1.0f + ((float)rand() / (float)RAND_MAX) * 2.0f;
    var /= 16.0f;

    return Eval + var;
}

static float en_evaluateOnePieceEndgamePosition(BoardState *BS) {
    // How close is opponent's king to the side?
    // How close are Pawns to queening?
    // How close is your king to your pawns?
    if (ch_getNumPiecesOnBoard(BS) == 2) return 0.0f;
    Coord BlackKing = {0};
    Coord WhiteKing = {0};
    bool BlackWinning = false;
    float PieceVal = 0.0f;
    for (int R = 0; R < BOARDSIZE; ++R) {
        for (int F = 0; F < BOARDSIZE; ++F) {
            char P = ch_pieceAt(BS, R, F);
            if (!P) continue;

            PieceVal += en_getPieceVal(P) * 4.0f;
            if (P == 'k')
                BlackKing = (Coord){R, F};
            else if (P == 'K')
                WhiteKing = (Coord){R, F};
            else if (ch_isBlackPiece(P))
                BlackWinning = true;
        }
    }
    float Eval = 0.0f;
    Coord HuntedKing = BlackKing;
    if (BlackWinning) {
        HuntedKing = WhiteKing;
    }
    int R = HuntedKing.Rank;
    int F = HuntedKing.File;
    float HalfBoard = (float)BOARDSIZE / 2.0f;
    float KingPosVal = (HalfBoard - ABS((float)R - HalfBoard)) +
        (HalfBoard - ABS((float)F - HalfBoard));
    BoardState BSCopy = *BS;
    BSCopy.BlackToMove = !BlackWinning;
    MoveList LegalMoves = ch_getLegalMoves(&BSCopy);
    Eval += KingPosVal;
    Eval += (float)LegalMoves.Count / 2.0f;

    return PieceVal + (BlackWinning ? Eval : -Eval);
}

Sequence en_recurrentEvaluateMove(BoardState *BS, Sequence Seq, int Depth, float Alpha, float Beta, float (*EvalFunc)(BoardState *)) {
    if (Depth == 0) {
        Seq.Eval = EvalFunc(BS);
        return Seq;
    }
    bool BlackToMove = BS->BlackToMove;

    Seq.Depth++;
    Sequence CurrBestSeq = Seq;
    CurrBestSeq.Eval = BlackToMove ? INFINITY : -INFINITY;
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
        Seq.Moves[Seq.Depth - 1] = Mv;
        Sequence CurrSeq = en_recurrentEvaluateMove(&BSCopy, Seq, Depth - 1, Alpha, Beta, EvalFunc);

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
    int Depth = 6;
    Sequence Seq = {0};
    if (ch_isOnePieceEndgame(BS))
        Seq = en_recurrentEvaluateMove(BS, (Sequence){0}, Depth + 2,
                -INFINITY, INFINITY, en_evaluateOnePieceEndgamePosition);
    else
        Seq = en_recurrentEvaluateMove(BS, (Sequence){0}, Depth,
                -INFINITY, INFINITY, en_evaluatePosition);
    return Seq;
}

#endif
