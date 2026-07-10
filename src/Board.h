#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "Types.h"

class Board 
{
    private: 
    BoardState gameState;
    public: 
    Board();
    void loadFEN(const std::string& fen); // text format that describes chess positions 
    void printBoard() const;
};

#endif