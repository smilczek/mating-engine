#include <assert.h>

#include "base.h"
#include "chess.h"
#include "chess.c"
#include "engine.c"

#include <stdio.h>
#include <string.h>

static const char *STARTING_POSITION_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static Move uci_parseMove(const char *S) {
    Move Mv = {0};

    Mv.From = ch_parseCoordinateStr(S);
    Mv.To = ch_parseCoordinateStr(S + 2);

    if (S[4] && S[4] != ' ' && S[4] != '\n' && S[4] != '\r') {
        Mv.Promotion = S[4];
    }
    return Mv;
}

static void uci_emitMove(Move Mv) {
    putchar(ch_fileToChar(Mv.From.File));
    putchar(ch_rankToChar(Mv.From.Rank));
    putchar(ch_fileToChar(Mv.To.File));
    putchar(ch_rankToChar(Mv.To.Rank));
    if (Mv.Promotion) {
        putchar(lowercase(Mv.Promotion));
    }
}

static void uci_handlePosition(BoardState *BS, char *Args) {
    if (strncmp(Args, "startpos", 8) == 0) {
        *BS = ch_parseFEN(STARTING_POSITION_FEN);
        Args += 8;
    } else if (strncmp(Args, "fen", 3) == 0) {
        Args += 3;
        while (*Args == ' ') ++Args;
        *BS = ch_parseFEN(Args);
    }

    char *Moves = strstr(Args, "moves");
    if (Moves) {
        Moves += 5;
        while (*Moves) {
            while (*Moves == ' ') ++Moves;
            Move Mv = uci_parseMove(Moves);
            ch_applyMove(BS, Mv);
            while (*Moves && *Moves != ' ') ++Moves;
        }
    }
}


static bool uci_isTokenChar(char c) {
    return c >= 'a' && c <= 'z';
}

static void logBoardState(BoardState *BS) {
    for (int Rank = BOARDSIZE - 1; Rank > -1; --Rank) {
        for (int File = 0; File < BOARDSIZE; ++File) {
            char Square = BS->Board[Rank][File];
            if (Square == '\0') {
                Square = '.';
            }
            fprintf(stderr, "%c", Square);
        }
        fprintf(stderr, "\n");
    }
}

static void logMove(BoardState *BS, Move Mv) {
    char P = uppercase(ch_pieceAtCoord(BS, Mv.From));
    if (P != 'P') {
        fprintf(stderr, "%c", P);
    }
    if (ch_pieceAtCoord(BS, Mv.To)) {
        if (P == 'P') {
            fprintf(stderr, "%c", ch_fileToChar(Mv.From.File));
        }
        fprintf(stderr, "x");
    }

    fprintf(stderr, "%c%c", ch_fileToChar(Mv.To.File), ch_rankToChar(Mv.To.Rank));
}
static void logSequence(BoardState *BS, Sequence *Seq) {
    for (int i = 0; i < Seq->Depth; ++i) {
        if (i % 2 == 0) {
            fprintf(stderr, "%d. ", i / 2 + 1);
        }
        logMove(BS, Seq->Moves[i]);
        fprintf(stderr, " ");
        ch_applyMove(BS, Seq->Moves[i]);
    }
    fprintf(stderr, "\n Depth: %d", Seq->Depth);
    fprintf(stderr, "\n Var Eval: %f\n", Seq->Eval);
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    bool Running = true;
    char InputBuf[4096] = {0};
    BoardState BS = {0};
    while (Running) {
        if (!fgets(InputBuf, sizeof(InputBuf), stdin)) break;
        if (strncmp(InputBuf, "uci", 3) == 0 && !uci_isTokenChar(InputBuf[3])) {
            printf("id name Mating Engine\n");
            printf("id author smilczek\n");
            printf("uciok\n");
        } else if (strncmp(InputBuf, "isready", 7) == 0) {
            printf("readyok\n");
        } else if (strncmp(InputBuf, "ucinewgame", 10) == 0) {
            BS = ch_parseFEN(STARTING_POSITION_FEN);
        } else if (strncmp(InputBuf, "position", 8) == 0) {
            char *Args = InputBuf + 8;
            while (*Args == ' ') ++Args;
            uci_handlePosition(&BS, Args);
            logBoardState(&BS);
        } else if (strncmp(InputBuf, "go", 2) == 0) {
            Sequence Seq = en_findBestSequence(&BS);
            BoardState BSCopy = BS;
            logSequence(&BSCopy, &Seq);
            printf("bestmove ");
            uci_emitMove(Seq.Moves[0]);
            printf("\n");
        } else if (strncmp(InputBuf, "quit", 4) == 0) {
            Running = false;
        }
    }

    return 0;
}
