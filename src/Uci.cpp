#include "Uci.h"
#include "Board.h"
#include "Search.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// the position a GUI gets if it never sends "position" before "go"
static const char* startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// play one coordinate move ("e2e4", "e7e8q") if it is legal in the current position
static bool applyUciMove(Board& board, const std::string& text)
{
    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    for (const Move& move : moves)
    {
        // moveToString is the same notation the GUI sends, including the promotion letter
        if (moveToString(move) == text)
        {
            board.makeMove(move);
            return true;
        }
    }

    return false;
}

// "position startpos [moves ...]" or "position fen <six fields> [moves ...]"
static void handlePosition(Board& board, std::istringstream& tokens)
{
    std::string token;
    tokens >> token;

    if (token == "startpos")
    {
        board.loadFEN(startFEN);
        // the next word is either "moves" or the end of the line
        tokens >> token;
    }
    else if (token == "fen")
    {
        // a FEN is six space-separated fields, and the move list starts at the word "moves"
        std::string fen;
        int fields = 0;
        while (tokens >> token && token != "moves")
        {
            if (!fen.empty())
            {
                fen += ' ';
            }
            fen += token;
            fields++;
        }

        // loadFEN needs every field; a short string would read off the end of the en passant token
        if (fields != 6)
        {
            return;
        }

        board.loadFEN(fen);
    }
    else
    {
        return;
    }

    if (token != "moves")
    {
        return;
    }

    std::string moveText;
    while (tokens >> moveText)
    {
        // stop at the first move we can't play so the rest aren't applied to the wrong position
        if (!applyUciMove(board, moveText))
        {
            break;
        }
    }
}

// the fields of "go" this engine understands. -1 means the GUI did not send that field.
struct GoCommand
{
    int depth = 0;
    int wtime = -1;
    int btime = -1;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    int movetime = -1;
};

static GoCommand parseGo(std::istringstream& tokens)
{
    GoCommand go;
    std::string token;
    while (tokens >> token)
    {
        if (token == "depth")
        {
            int value = 0;
            if (tokens >> value && value > 0)
            {
                go.depth = value;
            }
        }
        else if (token == "movetime")
        {
            int value = 0;
            if (tokens >> value && value >= 0)
            {
                go.movetime = value;
            }
        }
        else if (token == "wtime")
        {
            int value = 0;
            if (tokens >> value && value >= 0)
            {
                go.wtime = value;
            }
        }
        else if (token == "btime")
        {
            int value = 0;
            if (tokens >> value && value >= 0)
            {
                go.btime = value;
            }
        }
        else if (token == "winc")
        {
            int value = 0;
            if (tokens >> value && value >= 0)
            {
                go.winc = value;
            }
        }
        else if (token == "binc")
        {
            int value = 0;
            if (tokens >> value && value >= 0)
            {
                go.binc = value;
            }
        }
        else if (token == "movestogo")
        {
            int value = 0;
            if (tokens >> value && value > 0)
            {
                go.movestogo = value;
            }
        }
    }
    return go;
}

// how many milliseconds this move may think. -1 means the GUI sent no clock.
// with a clock, use about one share of the time left plus the increment,
// and never more than half of what is left, so one move cannot spend the game.
static int thinkTimeMs(Colors side, const GoCommand& go)
{
    if (go.movetime >= 0)
    {
        return go.movetime;
    }

    int remaining = (side == Colors::white) ? go.wtime : go.btime;
    if (remaining < 0)
    {
        return -1;
    }

    int increment = (side == Colors::white) ? go.winc : go.binc;
    int shares = go.movestogo > 0 ? go.movestogo : 20;
    int budget = remaining / shares + increment;
    int half = remaining / 2;
    if (budget > half)
    {
        budget = half;
    }
    if (budget < 0)
    {
        budget = 0;
    }
    return budget;
}

// set for the duration of one "go", so each finished depth can be printed
// from the side to move's point of view.
static Colors reportingSide = Colors::white;

static void reportDepth(const SearchResult& result, int elapsedMs)
{
    int score = result.score;
    if (reportingSide == Colors::black)
    {
        score = -score;
    }

    std::cout << "info depth " << result.depth
              << " score cp " << score
              << " nodes " << result.nodes
              << " time " << elapsedMs
              << " pv " << moveToString(result.bestMove) << '\n';
}

static void handleGo(const Board& board, std::istringstream& tokens)
{
    GoCommand go = parseGo(tokens);

    SearchLimits limits;
    limits.timeMs = thinkTimeMs(board.sideToMove(), go);
    limits.maxDepth = go.depth;
    // a bare "go" still looks 3 plies ahead, now by walking 1, then 2, then 3
    if (limits.timeMs < 0 && limits.maxDepth < 1)
    {
        limits.maxDepth = 3;
    }

    reportingSide = board.sideToMove();
    SearchResult result = iterativeSearch(board, limits, reportDepth);

    if (!result.hasMove)
    {
        // checkmate or stalemate: the GUI still expects a bestmove line
        std::cout << "bestmove (none)\n";
        return;
    }

    std::cout << "bestmove " << moveToString(result.bestMove) << '\n';
}

void uciLoop()
{
    // the GUI reads our output through a pipe, so every reply must be flushed right away
    // or it sits in the buffer and the GUI thinks the engine has hung
    std::cout << std::unitbuf;

    Board board;
    board.loadFEN(startFEN);

    std::string line;

    // getline fails when the GUI closes our input, which should end the engine just like "quit"
    while (std::getline(std::cin, line))
    {
        // GUIs on Windows can send \r\n line endings
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        // the first word is the command, the rest of the stream holds its arguments
        std::istringstream tokens(line);
        std::string command;
        tokens >> command;

        // blank lines are allowed and ignored
        if (command.empty())
        {
            continue;
        }

        if (command == "quit")
        {
            break;
        }

        if (command == "uci")
        {
            std::cout << "id name Chess Engine\n";
            std::cout << "id author Hadi\n";
            std::cout << "uciok\n";
            continue;
        }

        if (command == "isready")
        {
            std::cout << "readyok\n";
            continue;
        }

        // sent before a new game; there is no search state to clear yet, so just reset the board
        if (command == "ucinewgame")
        {
            board.loadFEN(startFEN);
            continue;
        }

        if (command == "position")
        {
            handlePosition(board, tokens);
            continue;
        }

        if (command == "go")
        {
            handleGo(board, tokens);
            continue;
        }

        // the UCI spec says unknown commands are ignored silently, so the engine
        // keeps running when a GUI sends something it doesn't support
    }
}
