#ifndef PERFT_H
#define PERFT_H

#include <cstdint>
#include "Board.h"

// counts every leaf node reachable from board in exactly depth plies
uint64_t perft(const Board& board, int depth);

// prints each root move with its own node count, then the total; diff this against
// another engine's divide output to find exactly which move is generated wrong
uint64_t perftDivide(const Board& board, int depth);

// runs the standard perft positions against their published node counts,
// returns true only if every one matches
bool runPerftSuite();

#endif
