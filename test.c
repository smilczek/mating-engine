#include <assert.h>
#include "base.h"
#include "chess.h"
#include "chess.c"
#include "string.h"

static bool test_lowercase() {
    bool Success = true;

    Success &= lowercase('c') == 'c';
    Success &= lowercase('C') == 'c';
    Success &= lowercase('.') == '.';
    Success &= lowercase('\0') == '\0';

    return Success;
}

static bool test_parseCoordinateStr() {
    bool Success = true;

    Coord Co = ch_parseCoordinateStr("a1");
    assert(Co.Rank == 0);
    assert(Co.File == 0);
    Success &= Co.Rank == 0;
    Success &= Co.File == 0;

    Co = ch_parseCoordinateStr("h8");
    assert(Co.Rank == 7);
    assert(Co.File == 7);
    Success &= Co.Rank == 7;
    Success &= Co.File == 7;

    Co = ch_parseCoordinateStr("e4");
    assert(Co.Rank == 3);
    assert(Co.File == 4);
    Success &= Co.Rank == 3;
    Success &= Co.File == 4;

    Co = ch_parseCoordinateStr("h5");
    assert(Co.Rank == 4);
    assert(Co.File == 7);
    Success &= Co.Rank == 4;
    Success &= Co.File == 7;

    return Success;
}

static bool test_parseFEN() {
    bool Success = true;

    // Starting position.
    BoardState BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Success &= memcmp(BS.Board, "RNBQKBNR"
                                "PPPPPPPP"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "pppppppp"
                                "rnbqkbnr", 64) == 0;
    Success &= BS.BlackToMove == false;
    Success &= BS.CR_WK &&
               BS.CR_WQ &&
               BS.CR_BK &&
               BS.CR_BQ;
    Success &= BS.EnPassant[0] == '\0';
    Success &= BS.EnPassant[1] == '\0';
    Success &= BS.HalfmoveClock == 0;
    Success &= BS.FullmoveNumber == 0;

    // En passant
    BS = ch_parseFEN("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
    Success &= memcmp(BS.Board, "\0\0\0\0K\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0pP\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0k\0\0\0", 64) == 0;
    Success &= BS.BlackToMove == true;
    Success &= BS.EnPassant[0] == 'e';
    Success &= BS.EnPassant[1] == '3';
    Success &= !(BS.CR_WK ||
                 BS.CR_WQ ||
                 BS.CR_BK ||
                 BS.CR_BQ);

    // Fullmove, Halfmove
    BS = ch_parseFEN("r1bqkb1r/pppppppp/2n2n2/8/8/2N2N2/PPPPPPPP/R1BQKB1R w KQkq - 4 3");
    Success &= memcmp(BS.Board, "R\0BQKB\0R"
                                "PPPPPPPP"
                                "\0\0N\0\0N\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0\0\0\0\0\0\0"
                                "\0\0n\0\0n\0\0"
                                "pppppppp"
                                "r\0bqkb\0r", 64) == 0;
    Success &= BS.HalfmoveClock == 4;
    Success &= BS.FullmoveNumber == 2;

    // Double digit parsing
    BS = ch_parseFEN("r1bqkb1r/pppppppp/2n2n2/8/8/2N2N2/PPPPPPPP/R1BQKB1R w KQkq - 40 25");
    Success &= BS.HalfmoveClock == 40;
    Success &= BS.FullmoveNumber == 24;

    return Success;
}

static bool test_inBoardBounds() {
    bool Success = true;

    Success &= ch_inBoardBounds(0, 0);
    Success &= ch_inBoardBounds(7, 7);
    Success &= !ch_inBoardBounds(8, 7);
    Success &= !ch_inBoardBounds(7, 8);
    Success &= !ch_inBoardBounds(-7, 8);
    Success &= !ch_inBoardBounds(1, -1);

    return Success;
}

static bool test_isFriendly() {
    bool Success = true;

    BoardState BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Success &= !ch_isFriendly(&BS, 'p');
    Success &= ch_isFriendly(&BS, 'K');
    Success &= ch_isEnemy(&BS, 'p');
    Success &= !ch_isEnemy(&BS, 'K');

    BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    Success &= !ch_isFriendly(&BS, 'Q');
    Success &= ch_isFriendly(&BS, 'n');
    Success &= ch_isEnemy(&BS, 'Q');
    Success &= !ch_isEnemy(&BS, 'n');

    return Success;
}

static bool test_isColorPiece() {
    bool Success = true;

    Success &= ch_isWhitePiece('P');
    Success &= ch_isWhitePiece('R');
    Success &= ch_isWhitePiece('N');
    Success &= ch_isWhitePiece('B');
    Success &= ch_isWhitePiece('Q');
    Success &= ch_isWhitePiece('K');

    Success &= !ch_isWhitePiece('p');
    Success &= !ch_isWhitePiece('r');
    Success &= !ch_isWhitePiece('n');
    Success &= !ch_isWhitePiece('b');
    Success &= !ch_isWhitePiece('q');
    Success &= !ch_isWhitePiece('k');

    Success &= ch_isBlackPiece('p');
    Success &= ch_isBlackPiece('r');
    Success &= ch_isBlackPiece('n');
    Success &= ch_isBlackPiece('b');
    Success &= ch_isBlackPiece('q');
    Success &= ch_isBlackPiece('k');

    Success &= !ch_isBlackPiece('P');
    Success &= !ch_isBlackPiece('R');
    Success &= !ch_isBlackPiece('N');
    Success &= !ch_isBlackPiece('B');
    Success &= !ch_isBlackPiece('Q');
    Success &= !ch_isBlackPiece('K');

    return Success;
}

static bool test_pieceAt() {
    bool Success = true;

    BoardState BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Success &= ch_pieceAtCoord(&BS, (Coord){0, 0}) == 'R';
    Success &= ch_pieceAtCoord(&BS, (Coord){0, 3}) == 'Q';
    Success &= ch_pieceAtCoord(&BS, (Coord){1, 3}) == 'P';
    Success &= ch_pieceAtCoord(&BS, (Coord){7, 7}) == 'r';
    Success &= ch_pieceAtCoord(&BS, (Coord){7, 6}) == 'n';
    Success &= ch_pieceAtCoord(&BS, (Coord){6, 7}) == 'p';

    return Success;
}

static bool test_generatePseudoLegalMoves() {
    bool Success = true;

    // Same results for black/white
    BoardState BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    MoveList MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 20;

    BS = ch_parseFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 20;

    BS = ch_parseFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 26;

    BS = ch_parseFEN("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 26;

    // Some endgame pos + no verify castling legality
    BS = ch_parseFEN("3qk3/4p3/8/8/8/8/8/R3K3 w Q - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 16;

    BS = ch_parseFEN("3qk3/4p3/8/8/8/8/8/R3K3 b Q - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 18;

    // Ignore your king in check
    BS = ch_parseFEN("4k3/8/1p1p1p2/8/1p1Q1p2/8/1p1p1p2/4K3 w - - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 21;

    // Pawn promotion black
    BS = ch_parseFEN("4k3/8/8/8/8/8/1p1p4/5K2 b - - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 13;

    // Pawn promotion + pawn promotion with taking
    BS = ch_parseFEN("2r2k2/3P4/8/8/8/8/8/5K2 w - - 0 1");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 13;

    // En Passant
    BS = ch_parseFEN("8/8/3k4/2pP4/3r4/8/5r1r/4K3 w - c6 0 2");
    MvL = ch_generatePseudoLegalMoves(&BS);
    Success &= MvL.Count == 6;

    return Success;
}

static bool test_filterLegalMoves() {
    bool Success = true;
    // Some endgame pos + verify castling legality
    BoardState BS = ch_parseFEN("3qk3/4p3/8/8/8/8/8/R3K3 w Q - 0 1");
    MoveList Pseudo = ch_generatePseudoLegalMoves(&BS);
    MoveList Legal = ch_filterLegalMoves(&BS, &Pseudo);
    Success &= Legal.Count == 13 && Legal.Count < Pseudo.Count;

    // En Passant
    BS = ch_parseFEN("8/8/3k4/2pP4/3r4/8/5r1r/4K3 w - c6 0 2");
    Pseudo = ch_generatePseudoLegalMoves(&BS);
    Legal = ch_filterLegalMoves(&BS, &Pseudo);
    Success &= Legal.Count == 1;

    // En Passant illegal
    BS = ch_parseFEN("4k3/8/4r3/3pP3/8/8/3r1r2/4K3 w - d6 0 2");
    Pseudo = ch_generatePseudoLegalMoves(&BS);
    Legal = ch_filterLegalMoves(&BS, &Pseudo);
    Success &= Legal.Count == 0;

    return Success;
}

int main() {
    bool Success = true;
    Success &= test_lowercase();
    assert(Success);
    Success &= test_parseCoordinateStr();
    assert(Success);
    Success &= test_parseFEN();
    assert(Success);
    Success &= test_inBoardBounds();
    assert(Success);
    Success &= test_isFriendly();
    assert(Success);
    Success &= test_isColorPiece();
    assert(Success);
    Success &= test_generatePseudoLegalMoves();
    assert(Success);
    Success &= test_filterLegalMoves();
    assert(Success);


    return Success;
}
