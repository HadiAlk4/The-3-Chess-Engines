# ♟️ C/C++ Chess Engine Development Roadmap

Welcome to the development roadmap for building a clean, medium-strength chess engine from scratch in C/C++! 

This repository follows an **incremental, milestone-driven architecture**. Each phase builds directly on the previous one, ensuring mathematical correctness and bug-free logic before adding AI complexity. 

## Measured strength

These numbers are from one run of the current `main` build. The Elo figures later in the roadmap are targets. This engine has not played a rated match, so it has no measured Elo.

It plays legal chess, finds short mates and hanging pieces, and a 4-ply search beats a 1-ply search of the same engine. The evaluation is material plus piece-square tables for pawns, knights, and the king, so the playing strength is tactical.

### Perft

`./engine perft` matched the published node counts on all 7 positions:

| Position | Depth | Nodes | Time |
| :--- | ---: | ---: | ---: |
| Start position | 5 | 4,865,609 | 255 ms |
| Kiwipete | 4 | 4,085,603 | 203 ms |
| Position 3 | 6 | 11,030,083 | 816 ms |
| Position 4 | 5 | 15,833,292 | 793 ms |
| Position 4 mirrored | 5 | 15,833,292 | 806 ms |
| Position 5 | 4 | 2,103,487 | 106 ms |
| Position 6 | 4 | 3,894,594 | 171 ms |

57,645,960 nodes in 3.15 seconds, 7/7 passed.

### Hash

`./engine hash` passed 7/7. The incremental Zobrist key matched a from-scratch rebuild through the start position to depth 4, Kiwipete to depth 3, and a promotion position to depth 3. Two move orders of the Italian Game produced the same key, and a one-step pawn move hashed differently from a double step because of the en passant file.

### Search speed

From the start position, scores in centipawns from White's side. Node counts include the shallower iterations.

| Command | Finished depth | Nodes | Time | Move | Score |
| :--- | ---: | ---: | ---: | :--- | ---: |
| `go depth 7` | 7 | 982,303 | 1,340 ms | d2d4 | +10 |
| `go movetime 1000` | 6 | 252,317 | 287 ms | d2d4 | +5 |

Depth 7 takes about 1.3 seconds, so a one-second search finishes depth 6 and does not finish depth 7.

### Tactics

| Position | Depth | Move | Score |
| :--- | ---: | :--- | ---: |
| `7k/5Q2/6K1/8/8/8/8/8 w - - 0 1` | 3 | f7f8 | 99999 |
| `r1bqkbnr/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4` | 3 | h5f7 | 99999 |
| `rnb1kbnr/pppppppp/8/8/8/q7/PPPPPPPP/RNBQKBNR w KQkq - 0 1` | 2 | b1a3 | 860 |
| `k7/8/1K6/8/8/8/8/7R w - - 0 1` | 3 | h1h8 | 99999 |

4/4. The first is mate by Qf8, the second is Scholar's mate, the third wins the queen on a3, and the fourth is Rh8 mate. A score of 99999 is mate in one.

### Depth 4 against depth 1

20 games, both sides this engine. One side searched 4 plies and the other searched 1 ply, colors swapped, from 10 openings: the start position, 1. e4, 1. d4, 1. Nf3, 1. c4, a Sicilian, an Open Game, a Queen's Gambit, a French, and an Italian. A game stopped when a side had no legal move, or at 80 plies after the opening.

Depth 4 won 12, lost 0, and 8 were still going at the ply cap. The search does not score repetitions or the 50-move rule, so those 8 are unfinished games. The result says 4 plies beats 1 ply on this program. It is not a rating against people or against another engine.

---

## 🏗️ Project Overview & Goals
* **Language:** C++ (with C-style performance optimizations where appropriate)
* **Estimated Size:** ~1,500 – 3,000 Lines of Code (LOC)
* **Target Strength:** Medium (~1500–1800 Elo once Phase 5 is complete)
* **Board Representation:** 1D Array (`int board[64]`) or `0x88` (Beginner/Medium friendly) OR Bitboards (`uint64_t`) for maximum speed.

---

## 🗓️ Roadmap & Milestones

### Phase 1: The Foundation (Board & State)
Before the engine can move, it must understand what a chessboard is and how to represent game states in memory.

- [✅] **Define Core Data Structures**
  - Implement the board representation (e.g., `int board[64]` or bitboard masks).
  - Create a `Move` struct/class (source square, target square, piece moved, captured piece, promotion flags).
  - Create a `BoardState` struct (current turn, castling rights, en passant target square, half-move clock for the 50-move rule).
- [✅] **FEN Parser (Forsyth Edwards Notation)**
  - Write a function to ingest standard FEN strings (e.g., `"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"`) and populate your internal board state. This is vital for unit testing specific puzzle positions.
- [✅] **ASCII Console Printer**
  - Create a debugging function that prints the 8x8 board to the console using readable ASCII characters (`P`, `n`, `b`, `r`, `q`, `k`, `.`) along with turn and castling indicators.

---

### Phase 2: Move Generation & Legal Validation (The Mechanical Core)
This is the most critical and time-consuming phase. Your engine must generate every legal move in any position with **zero rules bugs**.

- [✅] **Pseudo-Legal Move Generator**
  - **Sliders (Rook, Bishop, Queen):** Implement ray-casting loops that stop at board edges or obstacles.
  - **Leapers (Knight, King):** Implement fixed square-offset jumps.
  - **Pawns:** Implement single/double forward pushes and diagonal captures.
- [✅] **Special Rules**
  - **Castling:** Validate king/rook first-move rights, empty intervening squares, and ensure the King doesn't castle through check.
  - **En Passant:** Handle the unique diagonal capture of a pawn that just made a double-step.
  - **Promotions:** Allow pawns reaching the 8th/1st rank to promote to Q, R, B, or N.
- [✅] **Legal Move Filtering & King Safety**
  - Implement an `isSquareAttacked(int square, int attackerColor)` function.
  - Filter pseudo-legal moves by simulating each move on a temporary board state; if the moving side's King is left in check, discard the move.
- [✅] **The Perft (Performance Test) Suite — *CRITICAL STEP***
  - Build a recursive node-counter function (`perft(depth)`) to count total leaf nodes from a given position.
  - Verify your engine's Perft numbers against published mathematical constants (e.g., Starting Position Depth 5 = `4,865,609` nodes; "Kiwipete" position Depth 4 = `4,085,603` nodes).
  - 🚨 *Do not proceed to Phase 3 until your Perft suite passes with 100% accuracy!*

---

### Phase 3: Early UCI Protocol & GUI Integration
With move generation proven bulletproof, implement a basic translator so your engine can talk to standard chess GUIs (like Arena, CuteChess, or En Passant). At this stage, your engine will simply pick a **random legal move**, allowing you to visually debug on a real chessboard!

- [✅] **Standard I/O Command Loop**
  - Write a loop listening to `std::cin` for standard UCI text commands.
- [✅] **Implement Core UCI Commands**
  - `uci` → Respond with engine name, author, and `uciok`.
  - `isready` → Respond with `readyok`.
  - `position startpos moves e2e4 ...` / `position fen ...` → Parse the board state and apply the list of played moves.
  - `go ...` → Generate all legal moves, pick one at **random** (for now), and output `bestmove <move>` (e.g., `bestmove g1f3`).
  - `quit` → Cleanly exit the program.
- [✅] **GUI Setup & Visual Testing**
  - Load your compiled `.exe` / binary into a GUI and play a full game against your random-moving engine to confirm seamless communication.

---

### Phase 4: Basic AI & The Search Tree
Give the engine a brain. Now that it can communicate with a GUI, every improvement in this phase will be immediately noticeable in its playing strength.

- [✅] **Material Evaluation Function**
  - Write an `evaluate()` function returning a score in centipawns from White's perspective (or relative to the side to move).
  - Basic values: Pawn = `100`, Knight = `320`, Bishop = `330`, Rook = `500`, Queen = `900`.
- [✅] **Piece-Square Tables (PST)**
  - Add positional awareness using 8x8 integer grids. Reward Knights for central outposts (`+30`), penalize them on corners (`-50`), encourage pawns to advance, and push Kings to safety during the opening/middlegame.
- [✅] **The Minimax Algorithm**
  - Implement a recursive tree search that explores future moves to a fixed depth (e.g., Depth 3), maximizing the score for the engine and minimizing it for the opponent.
- [✅] **Alpha-Beta Pruning**
  - Add `alpha` and `beta` boundary variables to Minimax to instantly cut off branches where the opponent can force a refutation. This provides a ~10x–50x speedup, allowing real-time searches up to Depth 5 or 6!

---

### Phase 5: Smart Search & Optimization (Reaching Medium Strength)
To transition from a beginner bot to a sharp, tactical medium-strength engine, we optimize *how* the tree is searched.

- [✅] **Move Ordering (MVV-LVA)**
  - Alpha-Beta pruning is most efficient when best moves are searched first. Sort generated moves before searching using **Most Valuable Victim - Least Valuable Attacker** (e.g., search P × Q first, quiet moves last).
- [✅] **Quiescence Search**
  - Solve the "Horizon Effect" (where the engine searches to Depth 4, sees it won a piece, but misses that it gets recaptured on Depth 5).
  - When the main search hits depth limit, launch a mini-search that *only evaluates tactical captures* until the board reaches a "quiet" state.
- [✅] **Iterative Deepening & Time Management**
  - Replace fixed-depth searches with a loop that searches Depth 1, then Depth 2, Depth 3, etc.
  - Use C++ `std::chrono` timers to track elapsed time during the UCI `go wtime <ms> btime <ms>` command. If time expires midway through a deeper search, cleanly abort and return the best move found from the last completed depth.

---

### Phase 6: Advanced Polish & Optimization (Optional / Next Steps)
Once your engine reaches Phase 5, it will comfortably beat casual club players. To push it even further toward ~2000+ Elo:

- [✅] **Zobrist Hashing & Transposition Tables:** Store previously evaluated positions in a custom hash map to avoid redundant calculations when different move orders reach the same board state.
- [✅] **Null Move Pruning:** Give the opponent a "free turn" in the search tree; if our position is still winning even after skipping a move, we can safely prune the branch early.
- [✅] **Late Move Reductions (LMR):** Search moves that are unlikely to be good (like quiet moves late in the move ordering list) to a shallower depth to save CPU cycles.

---

## 📊 Suggested Development Timeline

| Phase | Core Focus | Estimated Pace | Key C++ Concepts |
| :--- | :--- | :--- | :--- |
| **Phase 1** | Board & FEN Parser | ~3 – 5 Days | Structs, Enums, String parsing (`std::stringstream`) |
| **Phase 2** | Move Gen & Perft | ~2 – 3 Weeks | Bitwise math, Array efficiency, Unit testing |
| **Phase 3** | Early UCI & GUI | ~2 – 3 Days | Standard I/O (`std::getline`), String tokenization |
| **Phase 4** | Alpha-Beta & Evaluation | ~1 Week | Recursion, Tree traversal, Clean math |
| **Phase 5** | Quiescence & Time Mgmt | ~1 – 2 Weeks | Algorithm optimization, `std::chrono` timers |
| **Phase 6** | Transposition Tables | Ongoing | Hash maps, Low-level memory management |

