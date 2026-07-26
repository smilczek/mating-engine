#ifndef _CHESS_C_
#define _CHESS_C_

static char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

static bool ch_isWhitePiece(char P) {
    return P >= 'A' && P <= 'Z';
}
static bool ch_isBlackPiece(char P) {
    return P >= 'a' && P <= 'z';
}
static bool ch_inBoardBounds(int R, int F) {
    return R >= 0 && R < BOARDSIZE && F >= 0 && F < BOARDSIZE;
}
static bool ch_isOOB(int R, int F) {
    return !ch_inBoardBounds(R, F);
}
static bool ch_isCoordOOB(Coord C) {
    return ch_isOOB(C.Rank, C.File);
}
static char ch_pieceAt(BoardState *BS, int R, int F) {
    return BS->Board[R][F];
}
static char ch_pieceAtCoord(BoardState *BS, Coord C) {
    return ch_pieceAt(BS, C.Rank, C.File);
}
static bool ch_isFriendly(BoardState *BS, char P) {
    if (P == '\0') {
        return false;
    }
    return BS->BlackToMove ? ch_isBlackPiece(P) : ch_isWhitePiece(P);
}
static bool ch_isEnemy(BoardState *BS, char P) {
    if (P == '\0') {
        return false;
    }
    return BS->BlackToMove ? ch_isWhitePiece(P) : ch_isBlackPiece(P);
}


// Parse coord like e4, b2 etc.
// 2 chars always.
// lowercase assumed.
// no verification.
static Coord ch_parseCoordinateStr(const char *CoordStr) {
    assert(CoordStr[0] >= 'a' && CoordStr[0] <= 'h');
    assert(CoordStr[1] >= '1' && CoordStr[1] <= '8');

    Coord Ret = {0};
    Ret.File = CoordStr[0] - 'a';
    Ret.Rank = CoordStr[1] - '1';
    return Ret;
}

// assumes null-terminated string.
// undef behavior for invalid FEN.
// TODO(smilczek): FEN validation (separate func).
static BoardState ch_parseFEN(const char *FENStr) {
    BoardState BS = {0};

    int Rank = 7; // 8
    int File = 0; // A
    while (*FENStr != ' ') {
        char Piece = *FENStr++;
        if (Piece >= '1' && Piece <= '8') {
            // Denotes how many empty squares there are to skip.
            File += Piece - '0';
            continue;
        }
        if (lowercase(Piece) >= 'a' && lowercase(Piece) <= 'z') {
            // Assume it's a valid piece (RNBKQ or rnbkq)
            BS.Board[Rank][File] = Piece;
            File++;
            continue;
        }

        // must be '/' character, denoting end of rank.
        Rank--;
        File = 0;
    }

    FENStr++;
    // w or b (which to move)
    switch (*FENStr) {
        case 'b': {
            BS.BlackToMove = true;
            break;
        }
        default: {
            break;
        }
    }

    FENStr++;
    // castling rights
    while (*(++FENStr) != ' ') {
        switch (*FENStr) {
            case 'K': {
                BS.CR_WK = true;
                break;
            }
            case 'Q': {
                BS.CR_WQ = true;
                break;
            }
            case 'k': {
                BS.CR_BK = true;
                break;
            }
            case 'q': {
                BS.CR_BQ = true;
                break;
            }
            default: {
                // Should be '-'
                break;
            }
        }
    }

    FENStr++;
    // En Passant target
    if (*FENStr != '-') {
        BS.EnPassant[0] = *FENStr++;
        BS.EnPassant[1] = *FENStr++;
    } else {
        FENStr++;
    }

    FENStr++;
    // Halfmove clock (for 50-move rule)
    BS.HalfmoveClock = 0;
    while (*FENStr != ' ') {
        BS.HalfmoveClock *= 10;
        char Digit = *FENStr++;
        BS.HalfmoveClock += Digit - '0';
    }

    FENStr++;
    // FullmoveNumber
    BS.FullmoveNumber = 0;
    while (*FENStr) {
        BS.FullmoveNumber *= 10;
        char Digit = *FENStr++;
        BS.FullmoveNumber += Digit - '0';
    }
    // FEN fullmove starts at 1. I want to keep ZII.
    BS.FullmoveNumber -= 1;

    return BS;
}

static void ch_addMove(MoveList* MvList, Move Mv) {
    assert(MvList->Count < 1000);
    MvList->List[MvList->Count++] = Mv;
}

// Can a Q|R|B|K slide to a square, assuming they could slide to the squares
// along the way.
// Meant to be used iteratively along a straight line.
// First call it for move 1 square away. Second, 2 squares. And so on.
// Up to the dev to use it correctly.
// Add a the move to MoveList if square unoccupied or occupied by enemy piece.
// Return false if square occupied or OOB. Signals to stop iterating.
static bool ch_trySlide(MoveList *MvList, BoardState *BS, Move Mv) {
    if (ch_isCoordOOB(Mv.To)) {
        return false;
    }
    char P = ch_pieceAtCoord(BS, Mv.To);
    if (!P) {
        ch_addMove(MvList, Mv);
        return true;
    }
    if (ch_isEnemy(BS, P)) {
        ch_addMove(MvList, Mv);
        return false;
    }
    return false;
}

static void ch_tryPawnTake(MoveList *MvList, BoardState *BS, Move Mv) {
    if (ch_isCoordOOB(Mv.To)) {
        return;
    }
    char P = ch_pieceAtCoord(BS, Mv.To);
    if (!P) {
        return;
    }
    if (ch_isEnemy(BS, P)) {
        ch_addMove(MvList, Mv);
        return;
    }
}

static bool ch_tryPawnMove(MoveList *MvList, BoardState *BS, Move Mv) {
    if (ch_isCoordOOB(Mv.To)) {
        return false;
    }
    char P = ch_pieceAtCoord(BS, Mv.To);
    if (P) {
        return false;
    }
    ch_addMove(MvList, Mv);
    return true;
}

// Assumes valid position.
static MoveList ch_generatePseudoLegalMoves(BoardState *BS) {
    MoveList MvList = {0};
    const char PromoPieces[4] = { BS->BlackToMove ? 'r' : 'R',
                                  BS->BlackToMove ? 'n' : 'N',
                                  BS->BlackToMove ? 'b' : 'B',
                                  BS->BlackToMove ? 'q' : 'Q' };

    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            char P = ch_pieceAt(BS, Rank, File);
            if (!ch_isFriendly(BS, P)) {
                continue;
            }
            P = lowercase(P);
            switch (P) {
                case 'b': {
                    for (int i = 0; i < arr_len(BISHOP_DIR_SET); ++i) {
                        Dir D = {BISHOP_DIR_SET[i][0], BISHOP_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        while (ch_trySlide(&MvList, BS, Mv)) {
                            Mv.To.Rank += D.R;
                            Mv.To.File += D.F;
                        }
                    }
                    break;
                }
                case 'r': {
                    for (int i = 0; i < arr_len(ROOK_DIR_SET); ++i) {
                        Dir D = {ROOK_DIR_SET[i][0], ROOK_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        while (ch_trySlide(&MvList, BS, Mv)) {
                            Mv.To.Rank += D.R;
                            Mv.To.File += D.F;
                        }
                    }
                    break;
                }
                case 'q': {
                    for (int i = 0; i < arr_len(BISHOP_DIR_SET); ++i) {
                        Dir D = {BISHOP_DIR_SET[i][0], BISHOP_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        while (ch_trySlide(&MvList, BS, Mv)) {
                            Mv.To.Rank += D.R;
                            Mv.To.File += D.F;
                        }
                    }
                    for (int i = 0; i < arr_len(ROOK_DIR_SET); ++i) {
                        Dir D = {ROOK_DIR_SET[i][0], ROOK_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        while (ch_trySlide(&MvList, BS, Mv)) {
                            Mv.To.Rank += D.R;
                            Mv.To.File += D.F;
                        }
                    }
                    break;
                }
                case 'k': {
                    for (int i = 0; i < arr_len(BISHOP_DIR_SET); ++i) {
                        Dir D = {BISHOP_DIR_SET[i][0], BISHOP_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        (void)ch_trySlide(&MvList, BS, Mv);
                    }
                    for (int i = 0; i < arr_len(ROOK_DIR_SET); ++i) {
                        Dir D = {ROOK_DIR_SET[i][0], ROOK_DIR_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        (void)ch_trySlide(&MvList, BS, Mv);
                    }
                    // Castling
                    bool CastleKingS = false;
                    bool CastleQueenS = false;
                    if (BS->BlackToMove) {
                        CastleKingS = BS->CR_BK;
                        CastleQueenS = BS->CR_BQ;
                    } else {
                        CastleKingS = BS->CR_WK;
                        CastleQueenS = BS->CR_WQ;
                    }

                    // Simple check, don't care about legality of castling yet.
                    if (CastleKingS) {
                        assert((BS->BlackToMove && Rank == 7) ||
                               (!BS->BlackToMove && Rank == 0));
                        // TODO(smilczek): Generalize this for chess960.
                        if (!(ch_pieceAt(BS, Rank, 5) || ch_pieceAt(BS, Rank, 6))) {
                            ch_addMove(&MvList, (Move){{Rank, File}, {Rank, 6}});
                        }
                    }
                    if (CastleQueenS) {
                        assert((BS->BlackToMove && Rank == 7) ||
                               (!BS->BlackToMove && Rank == 0));
                        // TODO(smilczek): Generalize this for chess960.
                        if (!(ch_pieceAt(BS, Rank, 1) || ch_pieceAt(BS, Rank, 2) ||
                              ch_pieceAt(BS, Rank, 3))) {
                            ch_addMove(&MvList, (Move){{Rank, File}, {Rank, 2}});
                        }
                    }
                    break;
                }
                case 'n': {
                    for (int i = 0; i < arr_len(KNIGHT_MOVE_SET); ++i) {
                        Dir D = {KNIGHT_MOVE_SET[i][0], KNIGHT_MOVE_SET[i][1]};
                        Move Mv = {{Rank, File}, {Rank + D.R, File + D.F}};
                        (void)ch_trySlide(&MvList, BS, Mv);
                    }
                    break;
                }
                case 'p': {
                    int PawnDir = BS->BlackToMove ? -1 : 1;
                    int PromoRank = BS->BlackToMove ? 0 : 7;
                    int BoostRank = BS->BlackToMove ? 6 : 1;

                    Move Mv = {{Rank, File}, {Rank + PawnDir, File - 1}};
                    if (Mv.To.Rank == PromoRank) {
                        for (int i = 0; i < arr_len(PromoPieces); ++i) {
                            Mv.Promotion = PromoPieces[i];
                            ch_tryPawnTake(&MvList, BS, Mv);
                        }
                    } else {
                        ch_tryPawnTake(&MvList, BS, Mv);
                    }
                    Mv = (Move){{Rank, File}, {Rank + PawnDir, File + 1}};
                    if (Mv.To.Rank == PromoRank) {
                        for (int i = 0; i < arr_len(PromoPieces); ++i) {
                            Mv.Promotion = PromoPieces[i];
                            ch_tryPawnTake(&MvList, BS, Mv);
                        }
                    } else {
                        ch_tryPawnTake(&MvList, BS, Mv);
                    }
                    Mv = (Move){{Rank, File}, {Rank + PawnDir, File}};
                    if (Mv.To.Rank == PromoRank) {
                        for (int i = 0; i < arr_len(PromoPieces); ++i) {
                            Mv.Promotion = PromoPieces[i];
                            (void)ch_tryPawnMove(&MvList, BS, Mv);
                        }
                    } else {
                        bool Blocked = !ch_tryPawnMove(&MvList, BS, Mv);
                        if (Rank == BoostRank && !Blocked) {
                            Mv.To.Rank += PawnDir;
                            (void)ch_tryPawnMove(&MvList, BS, Mv);
                        }
                    }
                    break;
                }
            }
        }
    }
    return MvList;
}

MoveList ch_filterLegalMoves(MoveList *Pseudo) {
    MoveList Legal = {0};

    // TODO(smilczek)

    return Legal;
}

#endif
