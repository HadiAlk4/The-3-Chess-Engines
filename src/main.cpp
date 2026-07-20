#include <iostream>
#include "Board.h"

int main() {
    Board board;
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    board.printBoard();

    return 0;
}