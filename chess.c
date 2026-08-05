#ifndef _CHESS_C_
#define _CHESS_C_

static char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}
static char uppercase(char c) {
    if (c >= 'a' && c <= 'z') {
        return c + 'A' - 'a';
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

static int ch_getNumWhitePieces(BoardState *BS) {
    int Num = 0;
    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            if (ch_isWhitePiece(BS->Board[Rank][File])) Num++;
        }
    }
    return Num;
}
static int ch_getNumBlackPieces(BoardState *BS) {
    int Num = 0;
    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            if (ch_isBlackPiece(BS->Board[Rank][File])) Num++;
        }
    }
    return Num;
}

static int ch_getNumPiecesOnBoard(BoardState *BS) {
    int Num = 0;
    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            if (BS->Board[Rank][File]) Num++;
        }
    }
    return Num;
}

bool ch_isOnePieceEndgame(BoardState *BS) {
    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            if (lowercase(BS->Board[Rank][File]) == 'p')
                return false;
        }
    }
    return ch_getNumPiecesOnBoard(BS) == 3;
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
    while (*FENStr && *FENStr >= '0' && *FENStr <= '9') {
        BS.FullmoveNumber *= 10;
        char Digit = *FENStr++;
        BS.FullmoveNumber += Digit - '0';
    }
    // FEN fullmove starts at 1. I want to keep ZII.
    BS.FullmoveNumber -= 1;

    return BS;
}

static void ch_addMove(MoveList* MvList, Move Mv) {
    assert(MvList->Count < MOVE_LIST_SIZE);
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
        if (BS->EnPassant[0]) {
            Coord EPSqr = ch_parseCoordinateStr(BS->EnPassant);
            if (Mv.To.File == EPSqr.File && Mv.To.Rank == EPSqr.Rank) {
                ch_addMove(MvList, Mv);
            }
        }
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

                    // Attack left
                    Move Mv = {{Rank, File}, {Rank + PawnDir, File - 1}};
                    if (Mv.To.Rank == PromoRank) {
                        for (int i = 0; i < arr_len(PromoPieces); ++i) {
                            Mv.Promotion = PromoPieces[i];
                            ch_tryPawnTake(&MvList, BS, Mv);
                        }
                    } else {
                        ch_tryPawnTake(&MvList, BS, Mv);
                    }
                    // Attack right
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

// Move is castling if the king jumps from File 4 to 2 or 6
static bool ch_moveIsCastling(BoardState *BS, Move Mv) {
    char P = BS->Board[Mv.From.Rank][Mv.From.File];
    if (lowercase(P) != 'k') {
        return false;
    }
    if (Mv.From.File == 4 && (Mv.To.File == 2 || Mv.To.File == 6)) {
        return true;
    }
    return false;
}

static char ch_rankToChar(int Rank) {
    return Rank + '1';
}
static char ch_fileToChar(int File) {
    return File + 'a';
}

static bool ch_moveIsEnPassant(BoardState *BS, Move Mv) {
    if (!BS->EnPassant[0]) {
        return false;
    }
    Coord EPCoord = ch_parseCoordinateStr(BS->EnPassant);
    if (Mv.To.Rank != EPCoord.Rank || Mv.To.File != EPCoord.File) {
        return false;
    }
    if (lowercase(ch_pieceAtCoord(BS, Mv.From)) == 'p') {
        return true;
    }
    return false;
}

// Assumes the move is at least pseudo-legal.
// Doesn't verify legality.
static void ch_applyMove(BoardState *BS, Move Mv) {
    char P = BS->Board[Mv.From.Rank][Mv.From.File];
    bool IsEnPassant = ch_moveIsEnPassant(BS, Mv);
    bool IsCastling = ch_moveIsCastling(BS, Mv);
    BS->Board[Mv.From.Rank][Mv.From.File] = '\0';
    BS->Board[Mv.To.Rank][Mv.To.File] = P;

    if (Mv.Promotion) {
        BS->Board[Mv.To.Rank][Mv.To.File] =
            BS->BlackToMove ? Mv.Promotion : uppercase(Mv.Promotion);
    }

    if (IsEnPassant) {
        // The pawn that used to be next to our pawn.
        BS->Board[Mv.From.Rank][Mv.To.File] = '\0';
    }

    BS->EnPassant[0] = '\0';
    BS->EnPassant[1] = '\0';

    if (lowercase(P) == 'p') {
        int Distance = Mv.To.Rank - Mv.From.Rank;
        int AbsDistance = Distance < 0 ? -Distance : Distance;
        char EnemyPawn = BS->BlackToMove ? 'P' : 'p';
        if (AbsDistance == 2) {
            if ((Mv.To.File != 0 && ch_pieceAt(BS, Mv.To.Rank, Mv.To.File - 1) == EnemyPawn) ||
                    (Mv.To.File != 7 &&ch_pieceAt(BS, Mv.To.Rank, Mv.To.File + 1) == EnemyPawn)) {
                int EPRank = Mv.To.Rank - (Distance / 2);
                BS->EnPassant[0] = ch_fileToChar(Mv.To.File);
                BS->EnPassant[1] = ch_rankToChar(EPRank);
            }
        }
    }

    switch (P) {
        case 'k': {
            BS->CR_BK = false;
            BS->CR_BQ = false;
            break;
        }
        case 'K': {
            BS->CR_WK = false;
            BS->CR_WQ = false;
            break;
        }
        case 'r': {
            // TODO(smilczek): Generalize this for chess960
            if (Mv.From.File == 0) {
                BS->CR_BQ = false;
            }
            if (Mv.From.File == 7) {
                BS->CR_BK = false;
            }
            break;
        }
        case 'R': {
            // TODO(smilczek): Generalize this for chess960
            if (Mv.From.File == 0) {
                BS->CR_WQ = false;
            }
            if (Mv.From.File == 7) {
                BS->CR_WK = false;
            }
            break;
        }
        default: {
            break;
        }
    }

    if (IsCastling) {
        if (Mv.To.File == 2) {
            int RookFromFile = 0;
            int RookToFile = 3;
            char Rook = BS->Board[Mv.From.Rank][RookFromFile];
            BS->Board[Mv.From.Rank][RookFromFile] = '\0';
            BS->Board[Mv.To.Rank][RookToFile] = Rook;
        } else {
            int RookFromFile = 7;
            int RookToFile = 5;
            char Rook = BS->Board[Mv.From.Rank][RookFromFile];
            BS->Board[Mv.From.Rank][RookFromFile] = '\0';
            BS->Board[Mv.To.Rank][RookToFile] = Rook;
        }
    }

    BS->BlackToMove = !BS->BlackToMove;
}

// ByBlack flag needed because we need to be able to check if a square is
// attacked after we've made a move.
static bool ch_squareIsAttacked(BoardState *BS, Coord Sqr, bool ByBlack) {
    // TODO(smilczek): Refactor this unmaintainable garbage. Fix code redundancy

    // Check if attacked by pawns, check if sees rook, bishop, knight.
    int Rank = Sqr.Rank;
    int File = Sqr.File;
    // Black pawns attack from above, white from below.
    {
        int EnemyPawnRankDir = ByBlack ? 1 : -1;
        Coord PawnRight = {Sqr.Rank + EnemyPawnRankDir, Sqr.File + 1};
        Coord PawnLeft = {Sqr.Rank + EnemyPawnRankDir, Sqr.File - 1};
        char AttackerPawn = ByBlack ? 'p' : 'P';
        if (!ch_isCoordOOB(PawnRight) &&
                ch_pieceAtCoord(BS, PawnRight) == AttackerPawn) {
            return true;
        }
        if (!ch_isCoordOOB(PawnLeft) &&
                ch_pieceAtCoord(BS, PawnLeft) == AttackerPawn) {
            return true;
        }
    }

    // Knight attacks
    for (int i = 0; i < arr_len(KNIGHT_MOVE_SET); ++i) {
        char AttackerKnight = ByBlack ? 'n' : 'N';
        Dir D = {KNIGHT_MOVE_SET[i][0], KNIGHT_MOVE_SET[i][1]};
        Coord KnightJump = {Rank + D.R, File + D.F};
        if (!ch_isCoordOOB(KnightJump) &&
                ch_pieceAtCoord(BS, KnightJump) == AttackerKnight) {
            return true;
        }
    }

    // King attacks
    for (int i = 0; i < arr_len(BISHOP_DIR_SET); ++i) {
        char AttackerKing = ByBlack ? 'k' : 'K';
        Dir D = {BISHOP_DIR_SET[i][0], BISHOP_DIR_SET[i][1]};
        Coord KingStep = {Rank + D.R, File + D.F};
        if (!ch_isCoordOOB(KingStep) &&
                ch_pieceAtCoord(BS, KingStep) == AttackerKing) {
            return true;
        }
    }
    for (int i = 0; i < arr_len(ROOK_DIR_SET); ++i) {
        char AttackerKing = ByBlack ? 'k' : 'K';
        Dir D = {ROOK_DIR_SET[i][0], ROOK_DIR_SET[i][1]};
        Coord KingStep = {Rank + D.R, File + D.F};
        if (!ch_isCoordOOB(KingStep) &&
                ch_pieceAtCoord(BS, KingStep) == AttackerKing) {
            return true;
        }
    }

    // Bishop/Queen
    for (int i = 0; i < arr_len(BISHOP_DIR_SET); ++i) {
        char AttackerBishop = ByBlack ? 'b' : 'B';
        char AttackerQueen = ByBlack ? 'q' : 'Q';
        Dir D = {BISHOP_DIR_SET[i][0], BISHOP_DIR_SET[i][1]};
        Coord BishopStep = {Rank + D.R, File + D.F};
        while (!ch_isCoordOOB(BishopStep)) {
            char P = ch_pieceAtCoord(BS, BishopStep);
            BishopStep.Rank += D.R;
            BishopStep.File += D.F;
            if (!P) {
                continue;
            }
            if (P == AttackerBishop || P == AttackerQueen) {
                return true;
            }
            break;
        }
    }

    // Rook/Queen
    for (int i = 0; i < arr_len(ROOK_DIR_SET); ++i) {
        char AttackerRook = ByBlack ? 'r' : 'R';
        char AttackerQueen = ByBlack ? 'q' : 'Q';
        Dir D = {ROOK_DIR_SET[i][0], ROOK_DIR_SET[i][1]};
        Coord RookStep = {Rank + D.R, File + D.F};
        while (!ch_isCoordOOB(RookStep)) {
            char P = ch_pieceAtCoord(BS, RookStep);
            RookStep.Rank += D.R;
            RookStep.File += D.F;
            if (!P) {
                continue;
            }
            if (P == AttackerRook || P == AttackerQueen) {
                return true;
            }
            break;
        }
    }

    return false;
}

static Coord ch_findKing(BoardState *BS, bool White) {
    char TheKing = White ? 'K' : 'k';
    for (int Rank = 0; Rank < BOARDSIZE; ++Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            if (ch_pieceAt(BS, Rank, File) == TheKing) {
                return (Coord){Rank, File};
            }
        }
    }
    assert(false);
    return (Coord){-1, -1};
}

static bool ch_moveIsLegal(BoardState *BS, Move Mv) {
    // White to move means we need to watch for attacking black pieces.
    bool BlackAttacks = !BS->BlackToMove;

    char P = BS->Board[Mv.From.Rank][Mv.From.File];
    if (ch_moveIsCastling(BS, Mv)) {
        // Can't castle out of check.
        if (ch_squareIsAttacked(BS, Mv.From, BlackAttacks)) {
            return false;
        }
        Coord JumpedSqr = {Mv.From.Rank, (Mv.From.File + Mv.To.File) / 2};
        if (ch_squareIsAttacked(BS, JumpedSqr, BlackAttacks)) {
            return false;
        }
    }
    BoardState BSCopy = *BS;
    ch_applyMove(&BSCopy, Mv);
    Coord KingCoord = ch_findKing(&BSCopy, BlackAttacks);
    if (ch_squareIsAttacked(&BSCopy, KingCoord, BlackAttacks)) {
        return false;
    }
    return true;
}

static bool ch_inCheck(BoardState *BS) {
    Coord KingCoord = ch_findKing(BS, !BS->BlackToMove);
    return ch_squareIsAttacked(BS, KingCoord, !BS->BlackToMove);
}

// Ignores en passant.
static bool ch_moveIsCapture(BoardState *BS, Move Mv) {
    return ch_pieceAtCoord(BS, Mv.To);
}

MoveList ch_filterLegalMoves(BoardState *BS, MoveList *Pseudo) {
    MoveList Legal = {0};
    MoveList Captures = {0};

    for (int i = 0; i < Pseudo->Count; ++i) {
        Move Mv = Pseudo->List[i];
        if (ch_moveIsLegal(BS, Mv)) {
            if (ch_moveIsCapture(BS, Mv)) {
                ch_addMove(&Captures, Mv);
            } else {
                ch_addMove(&Legal, Mv);
            }
        }
    }
    for (int i = 0; i < Legal.Count; ++i) {
        ch_addMove(&Captures, Legal.List[i]);
    }
    return Captures;
}

MoveList ch_getLegalMoves(BoardState *BS) {
    MoveList Pseudo = ch_generatePseudoLegalMoves(BS);
    MoveList Legal = ch_filterLegalMoves(BS, &Pseudo);

    return Legal;
}

#endif
