#include "Perft.h"
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <vector>

uint64_t perft(const Board& board, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    // every legal move here is a leaf, no need to play them out
    if (depth == 1)
    {
        return moves.size();
    }

    uint64_t nodes = 0;
    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        nodes += perft(next, depth - 1);
    }

    return nodes;
}

uint64_t perftDivide(const Board& board, int depth)
{
    if (depth < 1)
    {
        printf("\nNodes searched: 1\n");
        return 1;
    }

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    uint64_t total = 0;
    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        uint64_t nodes = perft(next, depth - 1);
        total += nodes;
        printf("%s: %" PRIu64 "\n", moveToString(move).c_str(), nodes);
    }

    printf("\nNodes searched: %" PRIu64 "\n", total);
    return total;
}

struct PerftCase
{
    const char* name;
    const char* fen;
    int depth;
    uint64_t expectedNodes;
};

// published reference counts from the Chess Programming Wiki "Perft Results" page;
// together they cover pins, checks, en passant, castling rights and promotions
static const PerftCase perftCases[] = {
    { "Start position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609ULL },
    { "Kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ULL },
    { "Position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083ULL },
    { "Position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292ULL },
    { "Position 4 mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292ULL },
    { "Position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487ULL },
    { "Position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594ULL },
};

bool runPerftSuite()
{
    int passed = 0;
    int total = sizeof(perftCases) / sizeof(perftCases[0]);

    for (const PerftCase& test : perftCases)
    {
        Board board;
        board.loadFEN(test.fen);

        auto start = std::chrono::steady_clock::now();
        uint64_t nodes = perft(board, test.depth);
        auto end = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        bool ok = nodes == test.expectedNodes;
        passed += ok;

        printf("[%s] %-20s depth %d  %12" PRIu64 " nodes (expected %12" PRIu64 ")  %6lld ms\n",
               ok ? "PASS" : "FAIL", test.name, test.depth, nodes, test.expectedNodes, ms);
    }

    printf("\n%d/%d perft positions passed\n", passed, total);
    return passed == total;
}
