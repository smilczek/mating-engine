#ifndef _CHESS_C_
#define _CHESS_C_

#include <assert.h>


static char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

// The board shall be so
// F,R
// 0,0 is A1,
// 0,7 is A8,
// 7,7 is H8.
// upper case means white
// lower case means black
static void initBoardState(BoardState *BS) {
    const char *BackrankSetup = "RNBQKBNR";
    const char *PawnSetup =     "PPPPPPPP";
    // TODO(smilczek) verify BackrankSetup is of BOARDSIZE length

    BS->FullmoveNumber = 0;
    BS->BlackToMove = 0;

    BS->CR_WK = true;
    BS->CR_WQ = true;
    BS->CR_BK = true;
    BS->CR_BQ = true;

    BS->EnPassant[0] = '\0';
    BS->EnPassant[1] = '\0';

    int Rank = 0;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = BackrankSetup[File];
    }
    Rank = 1;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = PawnSetup[File];
    }
    Rank = 7;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = lowercase(BackrankSetup[File]);
    }
    Rank = 6;
    for (int File = 0; File < BOARDSIZE; ++File) {
        BS->Board[Rank * BOARDSIZE + File] = lowercase(PawnSetup[File]);
    }
}

// Parse coord like e4, b2 etc.
// 2 chars always.
// lowercase assumed.
// no verification.
static Coord parseCoordinateStr(char *CoordStr) {
    assert(CoordStr[0] >= 'a' && CoordStr[0] <= 'h');
    assert(CoordStr[1] >= '1' && CoordStr[1] <= '8');

    Coord Ret = {0};
    Ret.Rank = CoordStr[0] - 'a';
    Ret.File = CoordStr[1] - '1';
    return Ret;
}

// assumes null-terminated string.
// undef behavior for invalid FEN.
// TODO(smilczek): FEN validation (separate func).
static BoardState parseFEN(char *FENStr) {
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
