#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <vector>
#include "Types.h"

class Board
{
    private:
    BoardState gameState; // private so direct bit manipulation cant happen
    void clearBoard();
    // walks each {rankStep, fileStep} ray from square until it hits an edge or a piece
    void addSliderMoves(int square, Pieces piece, const int directions[][2], int directionCount, std::vector<Move>& moves) const;
    // tries a single {rankStep, fileStep} jump per offset, skipping ones that leave the board or land on our own piece
    void addLeaperMoves(int square, Pieces piece, const int offsets[][2], int offsetCount, std::vector<Move>& moves) const;

    public: 
    Board();
    void loadFEN(const std::string& fen); // text format that describes chess positions 
    void printBoard() const;
    Pieces pieceAt(int square) const; // returns emptySquare if nothing is on the square
    void generateSliderMoves(std::vector<Move>& moves) const; // pseudo-legal rook, bishop and queen moves for the side to move
    void generateLeaperMoves(std::vector<Move>& moves) const; // pseudo-legal knight and king moves for the side to move (no castling)
};

std::string moveToString(const Move& move); // e.g. "a1a8"

#endif