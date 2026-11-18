#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "Board.h"
#include "Perft.h"
#include "Search.h"
#include "Uci.h"

static const char* startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// usage:
//   ./engine                         UCI mode, this is how a GUI launches the engine
//   ./engine print                   print the starting board
//   ./engine perft                   run the perft suite (exit code 1 on any failure)
//   ./engine divide <depth> [fen]    per-move node counts for debugging
//   ./engine eval [fen]              print the static evaluation in centipawns
//   ./engine search [depth] [fen]    minimax best move and White-relative score
//   ./engine hash                    check zobrist keys against a from-scratch rebuild

static bool playUci(Board& board, const char* text)
{
    std::vector<Move> moves;
    board.generateLegalMoves(moves);
    for (const Move& move : moves)
    {
        if (moveToString(move) == text)
        {
            board.makeMove(move);
            return true;
        }
    }
    return false;
}

// every makeMove has to leave the incremental hash equal to a full rebuild,
// including castling, en passant and promotions.
static bool hashWalk(const Board& board, int depth)
{
    if (!board.zobristValid())
    {
        return false;
    }
    if (depth == 0)
    {
        return true;
    }

    std::vector<Move> moves;
    board.generateLegalMoves(moves);
    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        if (!hashWalk(next, depth - 1))
        {
            return false;
        }
    }
    return true;
}

static bool hashCase(const char* name, bool ok)
{
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    return ok;
}

static bool runHashChecks()
{
    int passed = 0;
    int total = 0;

    Board start;
    start.loadFEN(startFEN);
    total++;
    passed += hashCase("start position rebuild", start.zobristValid());

    Board passedTurn = start;
    passedTurn.makeNullMove();
    Board blackToMove;
    blackToMove.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    total++;
    passed += hashCase("null move matches black to move",
                        passedTurn.zobristValid() && passedTurn.zobristKey() == blackToMove.zobristKey());

    // both lines end on Bc4, so the en passant file from e5 has been cleared
    // and the two move orders are the same position.
    Board lineA = start;
    Board lineB = start;
    const char* orderA[] = { "e2e4", "e7e5", "g1f3", "b8c6", "f1c4" };
    const char* orderB[] = { "g1f3", "b8c6", "e2e4", "e7e5", "f1c4" };
    bool transposed = true;
    for (const char* move : orderA)
    {
        transposed = transposed && playUci(lineA, move) && lineA.zobristValid();
    }
    for (const char* move : orderB)
    {
        transposed = transposed && playUci(lineB, move) && lineB.zobristValid();
    }
    total++;
    passed += hashCase("transposition e4/Nf3", transposed && lineA.zobristKey() == lineB.zobristKey());

    // a single step and a double step are different positions: only the double step
    // leaves an en passant file in the key.
    Board oneStep = start;
    Board twoStep = start;
    bool epDiffers = playUci(oneStep, "e2e3") && playUci(twoStep, "e2e4")
        && oneStep.zobristValid() && twoStep.zobristValid()
        && oneStep.zobristKey() != twoStep.zobristKey();
    total++;
    passed += hashCase("en passant file changes the key", epDiffers);

    struct Walk
    {
        const char* name;
        const char* fen;
        int depth;
    };
    const Walk walks[] = {
        { "start position walk", startFEN, 4 },
        { "kiwipete walk", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3 },
        { "promotions walk", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3 },
    };
    for (const Walk& walk : walks)
    {
        Board board;
        board.loadFEN(walk.fen);
        total++;
        passed += hashCase(walk.name, hashWalk(board, walk.depth));
    }

    printf("\n%d/%d hash checks passed\n", passed, total);
    return passed == total;
}

int main(int argc, char* argv[]) {
    std::string command = (argc > 1) ? argv[1] : "";

    if (command.empty()) {
        uciLoop();
        return 0;
    }

    if (command == "perft") {
        return runPerftSuite() ? 0 : 1;
    }

    if (command == "divide") {
        if (argc < 3) {
            printf("usage: %s divide <depth> [fen]\n", argv[0]);
            return 1;
        }

        Board board;
        board.loadFEN((argc > 3) ? argv[3] : startFEN);
        perftDivide(board, std::atoi(argv[2]));
        return 0;
    }

    if (command == "eval") {
        Board board;
        board.loadFEN((argc > 2) ? argv[2] : startFEN);
        printf("%d\n", board.evaluate());
        return 0;
    }

    if (command == "search") {
        if (argc > 2 && std::atoi(argv[2]) < 1) {
            printf("usage: %s search [depth] [fen]\n", argv[0]);
            return 1;
        }

        int depth = (argc > 2) ? std::atoi(argv[2]) : 3;
        Board board;
        board.loadFEN((argc > 3) ? argv[3] : startFEN);
        SearchResult result = minimaxSearch(board, depth);

        if (!result.hasMove) {
            printf("bestmove (none)\n");
            return 0;
        }

        printf("%s %d\n", moveToString(result.bestMove).c_str(), result.score);
        return 0;
    }

    if (command == "hash") {
        return runHashChecks() ? 0 : 1;
    }

    if (command == "print") {
        Board board;
        board.loadFEN(startFEN);
        board.printBoard();
        return 0;
    }

    printf("unknown command: %s\n", command.c_str());
    return 1;
}
