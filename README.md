# Mating Engine 2000

The best chess engine.

## Build
```bash
./build.sh
```
Requires gcc.

## Current state
### Move searching
Searches recursively through legal moves.
Uses simple Alpha Beta pruning.

### Depth
Depth set to 3 full moves unless in a single piece endgame.
Single piece endgames use 4 full moves depth.

### Eval funcs
Simple evaluation by counting pieces, with position on board taken into
account.

Single piece endgames use different evalution which encourages pushing the enemy
king to the edge of the board.

## Files
`chess.c` - chess board, moves verification etc.
`test.c` - tests for chess logic.
`test_engine.c` - tests for engine logic.
`uci.c` - UCI interface layer.

## TODO
- TODO.md
