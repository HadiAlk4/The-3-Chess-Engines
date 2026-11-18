#include "Search.h"

#include <algorithm>
#include <vector>

// bigger than any material or piece-square total, so a checkmate always outranks a capture.
// a shorter mate scores higher: mate in 1 is mateScore - 1, mate in 5 is mateScore - 5.
static const int mateScore = 100000;

// one past the largest mate score, so the root window never cuts off a real result.
static const int infinite = mateScore + 1;

// victim values, indexed by Pieces. the king is never captured.
static const int victimValue[7] = { 0, 100, 320, 330, 500, 900, 0 };
// cheaper attackers sort first: pawn, knight, bishop, rook, queen, king.
static const int attackerRank[7] = { 0, 1, 2, 3, 4, 5, 6 };

// Most Valuable Victim, Least Valuable Attacker. quiet moves score 0 and stay last.
// pawn takes queen is 8999, queen takes pawn is 995.
static int mvvLva(const Move& move)
{
    if (move.capturedPiece == Pieces::emptySquare)
    {
        return 0;
    }

    return victimValue[(int)move.capturedPiece] * 10 - attackerRank[(int)move.movedPiece];
}

static void orderMoves(std::vector<Move>& moves)
{
    // best captures first, so alpha-beta can refute the other moves and skip them
    std::stable_sort(moves.begin(), moves.end(), [](const Move& a, const Move& b)
    {
        return mvvLva(a) > mvvLva(b);
    });
}

// one node of the tree. ply is how many moves we have already played from the root,
// which is only used to prefer a faster mate.
// alpha is the best score White can already force. beta is the best score Black can already force.
// a score outside [alpha, beta] will never be chosen, so the rest of this node's moves are skipped.
static SearchResult minimax(const Board& board, int depth, int ply, int alpha, int beta)
{
    SearchResult result{};

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    // no legal move ends the game. check is a loss for the side about to move; otherwise it is stalemate.
    if (moves.empty())
    {
        if (board.isInCheck(board.sideToMove()))
        {
            int score = mateScore - ply;
            result.score = (board.sideToMove() == Colors::white) ? -score : score;
        }
        return result;
    }

    // depth ran out on a live position, so stop and ask the static evaluation
    if (depth == 0)
    {
        result.score = board.evaluate();
        return result;
    }

    orderMoves(moves);

    // White wants the highest score. Black wants the lowest, because the score is from White's point of view.
    bool maximizing = board.sideToMove() == Colors::white;
    bool found = false;

    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        SearchResult child = minimax(next, depth - 1, ply + 1, alpha, beta);

        bool better = !found || (maximizing ? child.score > result.score : child.score < result.score);
        if (better)
        {
            found = true;
            result.bestMove = move;
            result.score = child.score;
            result.hasMove = true;
        }

        if (maximizing)
        {
            if (result.score > alpha)
            {
                alpha = result.score;
            }
        }
        else if (result.score < beta)
        {
            beta = result.score;
        }

        // the opponent can already force a better result than anything left in this node
        if (alpha >= beta)
        {
            break;
        }
    }

    return result;
}

SearchResult minimaxSearch(const Board& board, int depth)
{
    if (depth < 1)
    {
        depth = 1;
    }

    return minimax(board, depth, 0, -infinite, infinite);
}
