#include "Search.h"

#include <algorithm>
#include <vector>

// bigger than any material or piece-square total, so a checkmate always outranks a capture.
// a shorter mate scores higher: mate in 1 is mateScore - 1, mate in 5 is mateScore - 5.
static const int mateScore = 100000;

// one past the largest mate score, so the root window never cuts off a real result.
static const int infinite = mateScore + 1;

// a capture sequence ends on its own, because each capture removes a piece.
// a chain of checks does not, so stop answering checks after this many extra plies.
static const int quiescenceCheckPly = 8;

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

// a capture or a promotion. the static eval is not trusted while one of these is pending.
static bool isTactical(const Move& move)
{
    return move.capturedPiece != Pieces::emptySquare
        || move.promotionPiece != Pieces::emptySquare;
}

static void orderMoves(std::vector<Move>& moves)
{
    // best captures first, so alpha-beta can refute the other moves and skip them
    std::stable_sort(moves.begin(), moves.end(), [](const Move& a, const Move& b)
    {
        return mvvLva(a) > mvvLva(b);
    });
}

// no legal move ends the game. check is a loss for the side about to move; otherwise it is stalemate.
static int gameOverScore(Colors sideToMove, int ply, bool inCheck)
{
    if (!inCheck)
    {
        return 0;
    }

    int mate = mateScore - ply;
    return (sideToMove == Colors::white) ? -mate : mate;
}

// keeps searching after the main depth runs out, but only while the position is noisy.
// a noisy position has a capture, a promotion, or a king in check.
// standing pat means "stop and take the static score". that is always allowed when
// not in check, so a bad capture can never score worse than doing nothing.
static int quiescence(const Board& board, int ply, int alpha, int beta, int checkPly, uint64_t& nodes)
{
    nodes++;

    bool maximizing = board.sideToMove() == Colors::white;
    bool inCheck = board.isInCheck(board.sideToMove());

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    if (moves.empty())
    {
        return gameOverScore(board.sideToMove(), ply, inCheck);
    }

    int standPat = board.evaluate();

    // not in check: the side to move may decline every capture and keep this score.
    // if that is already more than the opponent will allow, the captures cannot matter.
    if (!inCheck)
    {
        if (maximizing)
        {
            if (standPat >= beta)
            {
                return standPat;
            }
            if (standPat > alpha)
            {
                alpha = standPat;
            }
        }
        else
        {
            if (standPat <= alpha)
            {
                return standPat;
            }
            if (standPat < beta)
            {
                beta = standPat;
            }
        }

        std::vector<Move> tactical;
        for (const Move& move : moves)
        {
            if (isTactical(move))
            {
                tactical.push_back(move);
            }
        }
        moves.swap(tactical);

        // nothing left to capture or promote, so the position is quiet
        if (moves.empty())
        {
            return standPat;
        }
    }
    else if (checkPly == 0)
    {
        // still in check, but the extra check budget is spent. take the eval and stop.
        return standPat;
    }

    orderMoves(moves);

    // in check there is no "do nothing", so start from a score any legal reply can beat
    int best = inCheck ? (maximizing ? -infinite : infinite) : standPat;
    int nextCheckPly = inCheck ? checkPly - 1 : checkPly;

    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        int score = quiescence(next, ply + 1, alpha, beta, nextCheckPly, nodes);

        if (maximizing)
        {
            if (score > best)
            {
                best = score;
            }
            if (best > alpha)
            {
                alpha = best;
            }
        }
        else
        {
            if (score < best)
            {
                best = score;
            }
            if (best < beta)
            {
                beta = best;
            }
        }

        // the opponent can already force a better result than anything left in this node
        if (alpha >= beta)
        {
            break;
        }
    }

    return best;
}

// one node of the tree. ply is how many moves we have already played from the root,
// which is only used to prefer a faster mate.
// alpha is the best score White can already force. beta is the best score Black can already force.
// a score outside [alpha, beta] will never be chosen, so the rest of this node's moves are skipped.
static SearchResult minimax(const Board& board, int depth, int ply, int alpha, int beta, uint64_t& nodes)
{
    SearchResult result{};

    // depth ran out. keep going through captures so a hanging piece is not "won" on the last ply.
    if (depth == 0)
    {
        result.score = quiescence(board, ply, alpha, beta, quiescenceCheckPly, nodes);
        return result;
    }

    nodes++;

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    if (moves.empty())
    {
        result.score = gameOverScore(board.sideToMove(), ply, board.isInCheck(board.sideToMove()));
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
        SearchResult child = minimax(next, depth - 1, ply + 1, alpha, beta, nodes);

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

    uint64_t nodes = 0;
    SearchResult result = minimax(board, depth, 0, -infinite, infinite, nodes);
    result.depth = depth;
    result.nodes = nodes;
    return result;
}
