#include <cstdio>
#include <cstdlib>
#include <string>
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

    if (command == "print") {
        Board board;
        board.loadFEN(startFEN);
        board.printBoard();
        return 0;
    }

    printf("unknown command: %s\n", command.c_str());
    return 1;
}
