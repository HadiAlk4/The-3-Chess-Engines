#include <cstdint>
#include <array>



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

struct PieceOnSquare
{
    Pieces type = Pieces::emptySquare;
    Colors color = Colors::noColor;
};

struct BoardState
{
    std::array<PieceOnSquare, 64> board;

    Colors currentTurn;

    bool castleWhiteKing;
    bool castleWhiteQueen;
    bool castleBlackKing;
    bool castleBlackQueen;

    uint8_t enPassantTargetSquare = 200;


    uint8_t halfMoveCount = 0; // resets after pawn move or capture 
    uint8_t fullMoveCount = 1; // adds one everytime black finishes thier turn to track what turn the game is currently on 

};