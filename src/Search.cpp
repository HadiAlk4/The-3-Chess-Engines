#include "Search.h"

#include <algorithm>
#include <chrono>
#include <vector>

// bigger than any material or piece-square total, so a checkmate always outranks a capture.
// a shorter mate scores higher: mate in 1 is mateScore - 1, mate in 5 is mateScore - 5.
static const int mateScore = 100000;

// one past the largest mate score, so the root window never cuts off a real result.
static const int infinite = mateScore + 1;

// a capture sequence ends on its own, because each capture removes a piece.
// a chain of checks does not, so stop answering checks after this many extra plies.
static const int quiescenceCheckPly = 8;

// iterative deepening will not walk past this, even with a huge clock.
static const int absoluteMaxDepth = 64;

// the clock is read every 2048 nodes, so a search stops within a few milliseconds
// of the deadline without reading the timer on every position.
static const uint64_t timeCheckInterval = 2048;

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

// shared by every node of one search. armed is false for depth 1, so that
// depth always finishes and the engine still has a move when the clock is tiny.
struct SearchClock
{
    bool armed = false;
    bool stop = false;
    uint64_t nodes = 0;
    std::chrono::steady_clock::time_point deadline{};
};

// true once the clock has run out. the flag sticks, so the tree unwinds
// without starting any more moves.
static bool outOfTime(SearchClock& clock)
{
    clock.nodes++;

    if (clock.stop)
    {
        return true;
    }
    if (!clock.armed)
    {
        return false;
    }

    if (clock.nodes % timeCheckInterval == 0
        && std::chrono::steady_clock::now() >= clock.deadline)
    {
        clock.stop = true;
    }
    return clock.stop;
}

// the move that won the previous depth is tried first. alpha-beta then gets a
// strong score early and can skip more of the other moves.
static void preferMove(std::vector<Move>& moves, const Move& preferred)
{
    for (size_t i = 1; i < moves.size(); i++)
    {
        const Move& move = moves[i];
        if (move.sourceSquare == preferred.sourceSquare
            && move.targetSquare == preferred.targetSquare
            && move.promotionPiece == preferred.promotionPiece)
        {
            auto it = moves.begin() + static_cast<std::vector<Move>::difference_type>(i);
            std::rotate(moves.begin(), it, it + 1);
            return;
        }
    }
}

// a million slots. each one keeps the score of a position so a later visit,
// from a different move order, can reuse it. 64-bit keys make a false match rare,
// and a slot is only trusted when the full key matches.
static const size_t ttSlots = 1u << 20;
static const uint8_t ttExact = 0;
static const uint8_t ttLower = 1; // true score is at least the stored score
static const uint8_t ttUpper = 2; // true score is at most the stored score

// mates sit within a few plies of mateScore. anything past this is a real evaluation.
static const int mateBound = mateScore - 1000;

struct TTEntry
{
    uint64_t key = 0;
    int32_t score = 0;
    int16_t depth = -1;
    uint8_t flag = ttExact;
    uint8_t generation = 0;
    uint8_t from = 0;
    uint8_t to = 0;
    uint8_t promo = 0;
    uint8_t hasMove = 0;
};

static std::vector<TTEntry> transposition;
static uint8_t ttGeneration = 0;

// the stored mate distance is from this node, not from the root, so the same
// mate can be reused at a different depth in the tree.
static int scoreToTable(int score, int ply)
{
    if (score > mateBound)
    {
        return score + ply;
    }
    if (score < -mateBound)
    {
        return score - ply;
    }
    return score;
}

static int scoreFromTable(int score, int ply)
{
    if (score > mateBound)
    {
        return score - ply;
    }
    if (score < -mateBound)
    {
        return score + ply;
    }
    return score;
}

static uint8_t boundFlag(int score, int alpha, int beta)
{
    if (score <= alpha)
    {
        return ttUpper;
    }
    if (score >= beta)
    {
        return ttLower;
    }
    return ttExact;
}

struct TTLook
{
    bool cutoff = false;
    int score = 0;
    Move move{};
    bool hasMove = false;
};

static TTLook readTable(uint64_t key, int depth, int ply, int alpha, int beta)
{
    TTLook look;
    if (transposition.empty())
    {
        return look;
    }

    const TTEntry& entry = transposition[key & (transposition.size() - 1)];
    if (entry.depth < 0 || entry.key != key)
    {
        return look;
    }

    if (entry.hasMove)
    {
        look.hasMove = true;
        look.move.sourceSquare = entry.from;
        look.move.targetSquare = entry.to;
        look.move.promotionPiece = static_cast<Pieces>(entry.promo);
    }

    // a shallower search is not a proof for this depth, but its move is still
    // worth trying first.
    if (entry.depth < depth)
    {
        return look;
    }

    int score = scoreFromTable(entry.score, ply);
    bool proves = entry.flag == ttExact
        || (entry.flag == ttLower && score >= beta)
        || (entry.flag == ttUpper && score <= alpha);
    if (proves)
    {
        look.cutoff = true;
        look.score = score;
    }
    return look;
}

static void storeTable(uint64_t key, int depth, int ply, int score, uint8_t flag, const Move* move)
{
    if (transposition.empty())
    {
        return;
    }

    TTEntry& entry = transposition[key & (transposition.size() - 1)];
    bool samePosition = entry.key == key;
    // keep a deeper entry from this search. an older search loses its slot.
    if (entry.depth > depth && (samePosition || entry.generation == ttGeneration))
    {
        return;
    }

    entry.key = key;
    entry.score = scoreToTable(score, ply);
    entry.depth = static_cast<int16_t>(depth);
    entry.flag = flag;
    entry.generation = ttGeneration;
    entry.hasMove = move != nullptr;
    if (move != nullptr)
    {
        entry.from = move->sourceSquare;
        entry.to = move->targetSquare;
        entry.promo = static_cast<uint8_t>(move->promotionPiece);
    }
}

static void beginSearch()
{
    if (transposition.empty())
    {
        transposition.resize(ttSlots);
    }

    ttGeneration++;
    // the counter wrapped, so old slots would look current again. drop them.
    if (ttGeneration == 0)
    {
        std::fill(transposition.begin(), transposition.end(), TTEntry{});
        ttGeneration = 1;
    }
}

void clearSearchState()
{
    if (!transposition.empty())
    {
        std::fill(transposition.begin(), transposition.end(), TTEntry{});
    }
    ttGeneration = 0;
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
static int quiescence(const Board& board, int ply, int alpha, int beta, int checkPly, SearchClock& clock)
{
    // captures past the depth limit have to notice the clock too, or a long
    // recapture sequence can run past the time allowed for this move.
    if (outOfTime(clock))
    {
        return 0;
    }

    int alphaStart = alpha;
    int betaStart = beta;
    uint64_t key = board.zobristKey();
    TTLook remembered = readTable(key, 0, ply, alpha, beta);
    if (remembered.cutoff)
    {
        return remembered.score;
    }

    bool maximizing = board.sideToMove() == Colors::white;
    bool inCheck = board.isInCheck(board.sideToMove());

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    if (moves.empty())
    {
        int score = gameOverScore(board.sideToMove(), ply, inCheck);
        storeTable(key, 0, ply, score, ttExact, nullptr);
        return score;
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
                storeTable(key, 0, ply, standPat, ttLower, nullptr);
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
                storeTable(key, 0, ply, standPat, ttUpper, nullptr);
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
            storeTable(key, 0, ply, standPat, ttExact, nullptr);
            return standPat;
        }
    }
    else if (checkPly == 0)
    {
        // still in check, but the extra check budget is spent. take the eval and stop.
        // this is a guess, not a searched score, so it is not stored.
        return standPat;
    }

    orderMoves(moves);
    if (remembered.hasMove)
    {
        preferMove(moves, remembered.move);
    }

    // in check there is no "do nothing", so start from a score any legal reply can beat
    int best = inCheck ? (maximizing ? -infinite : infinite) : standPat;
    Move bestMove{};
    bool hasBest = false;
    int nextCheckPly = inCheck ? checkPly - 1 : checkPly;

    for (const Move& move : moves)
    {
        Board next = board;
        next.makeMove(move);
        int score = quiescence(next, ply + 1, alpha, beta, nextCheckPly, clock);
        if (clock.stop)
        {
            return 0;
        }

        if (maximizing)
        {
            if (score > best)
            {
                best = score;
                bestMove = move;
                hasBest = true;
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
                bestMove = move;
                hasBest = true;
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

    storeTable(key, 0, ply, best, boundFlag(best, alphaStart, betaStart), hasBest ? &bestMove : nullptr);
    return best;
}

// one node of the tree. ply is how many moves we have already played from the root,
// which is only used to prefer a faster mate.
// alpha is the best score White can already force. beta is the best score Black can already force.
// a score outside [alpha, beta] will never be chosen, so the rest of this node's moves are skipped.
static SearchResult minimax(const Board& board, int depth, int ply, int alpha, int beta,
                            SearchClock& clock, const Move* preferred, bool allowNull)
{
    SearchResult result{};

    // depth ran out. keep going through captures so a hanging piece is not "won" on the last ply.
    if (depth == 0)
    {
        result.score = quiescence(board, ply, alpha, beta, quiescenceCheckPly, clock);
        return result;
    }

    if (outOfTime(clock))
    {
        return result;
    }

    int alphaStart = alpha;
    int betaStart = beta;
    uint64_t key = board.zobristKey();
    TTLook remembered = readTable(key, depth, ply, alpha, beta);
    // the root needs a move it can play. an internal node can stop on a score alone.
    if (remembered.cutoff && (ply > 0 || remembered.hasMove))
    {
        result.score = remembered.score;
        result.bestMove = remembered.move;
        result.hasMove = remembered.hasMove;
        return result;
    }

    std::vector<Move> moves;
    board.generateLegalMoves(moves);

    bool inCheck = board.isInCheck(board.sideToMove());
    if (moves.empty())
    {
        result.score = gameOverScore(board.sideToMove(), ply, inCheck);
        storeTable(key, depth, ply, result.score, ttExact, nullptr);
        return result;
    }

    // White wants the highest score. Black wants the lowest, because the score is from White's point of view.
    bool maximizing = board.sideToMove() == Colors::white;

    // give the opponent a free move. if the position is still good enough to
    // beat the window, every real move would be too, so the rest of this node
    // can be skipped. a king and pawns is left out: passing can be the losing move.
    if (allowNull && ply > 0 && depth >= 3 && !inCheck && board.hasNonPawnMaterial(board.sideToMove()))
    {
        int standing = board.evaluate();
        bool alreadyGood = maximizing ? standing >= beta : standing <= alpha;
        if (alreadyGood)
        {
            int reduction = (depth >= 6) ? 3 : 2;
            int nullDepth = depth - 1 - reduction;
            if (nullDepth < 0)
            {
                nullDepth = 0;
            }

            Board next = board;
            next.makeNullMove();
            // a narrow window: we only care whether the skip clears the cutoff.
            int nullAlpha = maximizing ? beta - 1 : alpha;
            int nullBeta = maximizing ? beta : alpha + 1;
            SearchResult skipped = minimax(next, nullDepth, ply + 1, nullAlpha, nullBeta, clock, nullptr, false);
            if (clock.stop)
            {
                return result;
            }

            if (maximizing && skipped.score >= beta)
            {
                result.score = beta;
                storeTable(key, depth, ply, beta, ttLower, nullptr);
                return result;
            }
            if (!maximizing && skipped.score <= alpha)
            {
                result.score = alpha;
                storeTable(key, depth, ply, alpha, ttUpper, nullptr);
                return result;
            }
        }
    }

    orderMoves(moves);
    // the move that worked in an earlier search goes first, then the previous
    // iteration's choice at the root, which wins if both are set.
    if (remembered.hasMove)
    {
        preferMove(moves, remembered.move);
    }
    if (ply == 0 && preferred != nullptr)
    {
        preferMove(moves, *preferred);
    }

    bool found = false;

    for (const Move& move : moves)
    {
        // between root moves, stop as soon as the clock is up. the moves already
        // searched here are not the whole list, so this depth will be discarded.
        if (ply == 0 && clock.armed && std::chrono::steady_clock::now() >= clock.deadline)
        {
            clock.stop = true;
            return result;
        }

        Board next = board;
        next.makeMove(move);
        SearchResult child = minimax(next, depth - 1, ply + 1, alpha, beta, clock, nullptr, true);
        if (clock.stop)
        {
            return result;
        }

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

    storeTable(key, depth, ply, result.score, boundFlag(result.score, alphaStart, betaStart),
               result.hasMove ? &result.bestMove : nullptr);
    return result;
}

SearchResult minimaxSearch(const Board& board, int depth)
{
    if (depth < 1)
    {
        depth = 1;
    }

    SearchClock clock;
    beginSearch();
    SearchResult result = minimax(board, depth, 0, -infinite, infinite, clock, nullptr, true);
    result.depth = depth;
    result.nodes = clock.nodes;
    return result;
}

static int elapsedMs(std::chrono::steady_clock::time_point start)
{
    return (int)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
}

SearchResult iterativeSearch(const Board& board, const SearchLimits& limits, DepthFinishedFn onDepth)
{
    int depthLimit = limits.maxDepth;
    if (depthLimit < 1 || depthLimit > absoluteMaxDepth)
    {
        depthLimit = absoluteMaxDepth;
    }

    // timeMs < 0 is an untimed search. zero still finishes depth 1 and then stops.
    bool timed = limits.timeMs >= 0;
    auto start = std::chrono::steady_clock::now();
    auto deadline = start + std::chrono::milliseconds(timed && limits.timeMs > 0 ? limits.timeMs : 0);

    SearchClock clock;
    clock.deadline = deadline;
    // one table for every depth, so depth 4 can reuse what depth 3 already proved.
    beginSearch();

    SearchResult finished{};
    Move previousBest{};
    bool hasPrevious = false;

    for (int depth = 1; depth <= depthLimit; depth++)
    {
        if (timed && depth > 1 && std::chrono::steady_clock::now() >= deadline)
        {
            break;
        }

        clock.armed = timed && depth > 1;
        clock.stop = false;

        const Move* preferred = hasPrevious ? &previousBest : nullptr;
        SearchResult result = minimax(board, depth, 0, -infinite, infinite, clock, preferred, true);

        // a depth that stops halfway has only scored some of its moves, so its
        // best move is not trustworthy. keep the last depth that finished.
        if (clock.stop)
        {
            break;
        }

        result.depth = depth;
        result.nodes = clock.nodes;
        finished = result;

        if (onDepth != nullptr && finished.hasMove)
        {
            onDepth(finished, elapsedMs(start));
        }

        if (!finished.hasMove)
        {
            break;
        }

        previousBest = finished.bestMove;
        hasPrevious = true;
    }

    return finished;
}
