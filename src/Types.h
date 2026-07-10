#include <cstdint>

using bitBoard = uint64_t;

enum class Pieces : uint8_t
{
    emptySquare,
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king
};

enum class Colors : uint8_t
{
    white,
    black,
    noColor
};

struct Move
{
    uint8_t sourceSquare;
    uint8_t targetSquare;

    Pieces movedPiece;
    Pieces capturedPiece;
    Pieces promotionPiece;

    bool wasCastling;
    bool wasEnPassant;
};



struct BoardState
{

    bitBoard pieceMasks[7] = {0};
    bitBoard colorMasks[2] = {0};

    Colors currentTurn;

    bool castleWhiteKing;
    bool castleWhiteQueen;
    bool castleBlackKing;
    bool castleBlackQueen;

    uint8_t enPassantTargetSquare = 200;


    uint8_t halfMoveCount = 0; // resets after pawn move or capture 
    uint8_t fullMoveCount = 1; // adds one everytime black finishes thier turn to track what turn the game is currently on 

};