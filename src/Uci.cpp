#include "Uci.h"
#include "Board.h"

#include <iostream>
#include <random>
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

// "go" ignores time and depth for now and answers with one random legal move
static void handleGo(const Board& board)
{
    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    if (moves.empty())
    {
        // checkmate or stalemate: the GUI still expects a bestmove line
        std::cout << "bestmove (none)\n";
        return;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> pick(0, static_cast<int>(moves.size()) - 1);
    std::cout << "bestmove " << moveToString(moves[pick(rng)]) << '\n';
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
            handleGo(board);
            continue;
        }

        // the UCI spec says unknown commands are ignored silently, so the engine
        // keeps running when a GUI sends something it doesn't support
    }
}
