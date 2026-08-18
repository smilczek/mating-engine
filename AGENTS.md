# INSTRUCTIONS FOR AI AGENTS

## Workflow
1. Read this file and pick the next unfinished task from the TODO list below.
2. Write failing tests in `test_bitboard.c` that exercise the new functionality.
3. Implement the feature in `bitboard.c` until all tests pass.
4. Run `./build.sh` and verify tests pass.
5. Commit your changes with a descriptive message: `git add bitboard.c test_bitboard.c && git commit -m "feat(bitboard): <short description>"`
6. Mark the completed task in this file by replacing `[ ]` with `[x]`.

## Rules
- **TDD is mandatory**: tests must be written BEFORE implementation.
- One task per session. Do not attempt multiple tasks at once.
- Keep `bitboard.c` self-contained (header-guarded like `chess.c`).
- Reference `chess.c` for expected behavior — the bitboard version must produce identical results.
- Reference Stockfish's bitboard/attacks design for patterns (magic bitboards, popcount, LSB iteration).
- Do NOT touch `chess.c`, `engine.c`, or `main.c` unless the task explicitly requires it.
- Use `__builtin_popcountll`, `__builtin_ctzll`, `__builtin_clzll` for bit ops (GCC/Clang).
- All functions prefixed with `bb_`. Tests prefixed with `test_`.

# TODO

## Bitboard Infrastructure
[x] 1. Add `bb_Square()` — create bitboard for a single square (maps Stockfish's `square_bb`)
[x] 2. Add `bb_popcount()` — count set bits using `__builtin_popcountll`
[x] 3. Add `bb_lsb()` / `bb_msb()` — get least/most significant bit index (De Bruijn or `__builtin_ctzll`)
[x] 4. Add `bb_pop_lsb()` — find and clear LSB; enables iterating all set squares in a bitboard
[x] 5. Add `bb_moreThanOne()` — test if bitboard has >1 bit set
[x] 6. Add `bb_nextBit()` — iterate bits via `pop_lsb` loop pattern

## State Representation
[x] 7. Expand `BitboardState` to use 12 bitboards (6 piece types × 2 colors) + occupied = 13 total, like Stockfish's occupancy by color/piece
[x] 8. Add combined occupancy bitboards: `AllPieces`, `Occupancy[2]` (white/black), `Blocked` (either color)
[x] 9. Replace `EnPassant` bitboard with single-square representation (matches Stockfish)
[x] 10. Convert castling rights to a compact integer encoding (4 bits) instead of 4 bools

## FEN Parsing & Display
[x] 11. Implement `bb_parseFEN()` — populate bitboards from FEN string
[ ] 12. Implement `bb_fenToString()` — serialize bitboard state back to FEN
[ ] 13. Add `bb_printBoard()` — ASCII debug visualization of bitboard state

## Attack Generation (precomputed tables)
[ ] 14. Create `BB_PseudoAttacks_Knight[64]` — precomputed knight attack bitboards per square
[ ] 15. Create `BB_PseudoAttacks_King[64]` — precomputed king attack bitboards per square
[ ] 16. Create `BB_PawnAttacks[2][64]` — pawn attack masks per color per square
[ ] 17. Create `BB_PawnPushes[2][64]` — legal single/double push squares per color per square
[ ] 18. Create `BB_PseudoAttacks_Bishop[64]` / `BB_PseudoAttacks_Rook[64]` — max attack coverage on empty board

## Sliding Piece Attacks (magic bitboards)
[ ] 19. Implement `bb_slidingAttack_bishop(sq, occupied)` — naive iterative version first
[ ] 20. Implement `bb_slidingAttack_rook(sq, occupied)` — naive iterative version first
[ ] 21. Implement `bb_slidingAttack_queen(sq, occupied)` — combine bishop + rook
[ ] 22. Implement magic bitboard tables for bishop attacks (`BB_MagicBishop[64]`)
[ ] 23. Implement magic bitboard tables for rook attacks (`BB_MagicRook[64]`)
[ ] 24. Write `bb_initMagics()` — runtime initialization of magic bitboard lookup tables

## Geometry Tables
[ ] 25. Build `BB_Line[64][64]` — squares on the line between two squares (rank/file/diag)
[ ] 26. Build `BB_Between[64][64]` — squares strictly between two squares
[ ] 27. Build `BB_RayPass[64][64]` — squares beyond s2 going from s1

## Move Generation
[ ] 28. Implement `bb_genKnightMoves(pieceBB, enemyBB)` — all knight moves via bitboard ops
[ ] 29. Implement `bb_genKingMoves(kingSq, friendlyBB, enemyBB)` — all king moves
[ ] 30. Implement `bb_genBishopMoves(bishopBB, allOccBB, enemyBB)` — sliding moves with captures separated
[ ] 31. Implement `bb_genRookMoves(rookBB, allOccBB, enemyBB)` — sliding moves with captures separated
[ ] 32. Implement `bb_genQueenMoves(queenBB, allOccBB, enemyBB)` — combined sliding
[ ] 33. Implement `bb_genPawnMoves(pawnBB, enemyBB, allOccBB)` — pushes, captures, promotions, en passant
[ ] 34. Implement `bb_genCastlingMoves()` — king-side and queen-side castling
[ ] 35. Implement `bb_genPseudoLegalMoves()` — full pseudo-legal move generator returning bitboards

## Move Encoding
[ ] 36. Design bit-packed `Move` type (e.g., 15 bits: 6 from + 6 to + 3 promo + 1 flag)
[ ] 37. Implement `bb_encodeMove(from, to, promotion, flags)` / `bb_decodeMove()`
[ ] 38. Implement `bb_moveIsCastling()`, `bb_moveIsEnPassant()`, `bb_moveIsPromotion()`

## Move Application
[ ] 39. Implement `bb_applyMove()` — update all bitboards, castling rights, en passant, halfmove clock
[ ] 40. Implement `bb_unapplyMove()` — reversible move for search (critical for performance)

## Check / Legality
[ ] 41. Implement `bb_isSquareAttacked(sq, attackerColor, allOccBB, ...pieceBBs...)`
[ ] 42. Implement `bb_inCheck()` — is the side to move in check
[ ] 43. Implement `bb_filterLegalMoves()` — remove moves that leave king in check
[ ] 44. Implement `bb_getLegalMoves()` — full legal move generation pipeline
[ ] 45. Implement `bb_isCheckmate()` / `bb_isStalemate()` — terminal position detection

## Tests
[ ] 46. Extend `test_bitboard.c` — tests for popcount, lsb/msb, pop_lsb iteration
[ ] 47. Add tests for all shift directions on all 64 squares
[ ] 48. Add tests for FEN parse/serialize round-trip
[ ] 49. Add tests for attack tables vs brute-force per-square verification
[ ] 50. Add move generation parity tests: compare `chess.c` output vs `bitboard.c` output on benchmark positions
[ ] 51. Add legality filter parity tests against `chess.c`