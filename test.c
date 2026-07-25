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

    Coord Co = parseCoordinateStr("a1");
    assert(Co.Rank == 0);
    assert(Co.File == 0);
    Success &= Co.Rank == 0;
    Success &= Co.File == 0;

    Co = parseCoordinateStr("h8");
    assert(Co.Rank == 7);
    assert(Co.File == 7);
    Success &= Co.Rank == 7;
    Success &= Co.File == 7;

    Co = parseCoordinateStr("e4");
    assert(Co.Rank == 4);
    assert(Co.File == 3);
    Success &= Co.Rank == 4;
    Success &= Co.File == 3;

    Co = parseCoordinateStr("h5");
    assert(Co.Rank == 7);
    assert(Co.File == 4);
    Success &= Co.Rank == 7;
    Success &= Co.File == 4;

    return Success;
}

static bool test_parseFEN() {
    bool Success = true;

    // Starting position.
    BoardState BS = parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
    BS = parseFEN("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
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
    BS = parseFEN("r1bqkb1r/pppppppp/2n2n2/8/8/2N2N2/PPPPPPPP/R1BQKB1R w KQkq - 4 3");
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
    BS = parseFEN("r1bqkb1r/pppppppp/2n2n2/8/8/2N2N2/PPPPPPPP/R1BQKB1R w KQkq - 40 25");
    Success &= BS.HalfmoveClock == 40;
    Success &= BS.FullmoveNumber == 24;

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

    return Success;
}
