#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"

// best move from a fixed-depth minimax search.
// score is centipawns from White's perspective, the same scale as Board::evaluate().
// hasMove is false when the side to move is already checkmated or stalemated.
struct SearchResult
{
    Move bestMove;
    int score;
    bool hasMove;
};

// White maximizes the score, Black minimizes it, with alpha-beta pruning.
// depth is in plies (a full move is 2). At depth 0, captures keep being
// searched until the position is quiet, so a capture on the horizon is
// not scored until the recapture is seen.
SearchResult minimaxSearch(const Board& board, int depth);

#endif
