#ifndef _CHESS_C_
#define _CHESS_C_

#include <assert.h>


static char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
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
            BS.Board[Rank * BOARDSIZE + File] = Piece;
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

#endif
