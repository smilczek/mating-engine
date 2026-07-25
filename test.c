#include "chess.c"

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

int main() {
    bool Success = true;
    Success &= test_parseCoordinateStr();
    Success &= test_lowercase();

    return Success;
}
