#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"

// best move from a search.
// score is centipawns from White's perspective, the same scale as Board::evaluate().
// hasMove is false when the side to move is already checkmated or stalemated.
// depth is the deepest search that ran to completion. nodes counts the positions
// visited through that depth.
struct SearchResult
{
    Move bestMove;
    int score;
    bool hasMove;
    int depth;
    uint64_t nodes;
};

// limits for one search.
// maxDepth < 1 means no depth cap (the loop still stops at 64 plies).
// timeMs < 0 means no clock. timeMs of 0 still finishes depth 1, then stops.
struct SearchLimits
{
    int maxDepth = 0;
    int timeMs = -1;
};

// called after each depth that finishes, with how many milliseconds have passed.
using DepthFinishedFn = void (*)(const SearchResult& result, int elapsedMs);

// White maximizes the score, Black minimizes it, with alpha-beta pruning.
// depth is in plies (a full move is 2). At depth 0, captures keep being
// searched until the position is quiet, so a capture on the horizon is
// not scored until the recapture is seen.
// A position reached by two different move orders is searched once and then
// read back from the transposition table.
// This searches exactly `depth` plies and ignores the clock.
SearchResult minimaxSearch(const Board& board, int depth);

// depth 1, then depth 2, then depth 3, and so on.
// if the clock runs out in the middle of a depth, that depth is discarded
// and the move from the last finished depth is returned.
SearchResult iterativeSearch(const Board& board, const SearchLimits& limits, DepthFinishedFn onDepth = nullptr);

// drops remembered positions. a new game must not reuse the previous game's scores.
void clearSearchState();

#endif
