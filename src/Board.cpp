#include <stdio.h>
#include "Board.h"
#include <cctype>
#include <sstream>

// {rankStep, fileStep}
const int rookDirections[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
const int bishopDirections[4][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };

void Board::clearBoard()
{
    for(int i=0;i<7;i++)
    {
        gameState.pieceMasks[i]=0;
    }

    gameState.colorMasks[0]=0;
    gameState.colorMasks[1]=0;

    gameState.currentTurn = Colors::white;
    gameState.castleBlackKing = false;
    gameState.castleBlackQueen = false;
    gameState.castleWhiteKing = false;
    gameState.castleWhiteQueen = false;

    gameState.enPassantTargetSquare = 200;
    gameState.halfMoveCount = 0;
    gameState.fullMoveCount = 1;

}

Board::Board() { 
    // Kept for now: if refactored in Types.h can be removed
    for (int i = 0; i < 7; i++) {
        gameState.pieceMasks[i] = 0;
    }    
      
    gameState.colorMasks[0] = 0;
    gameState.colorMasks[1] = 0;
    
    gameState.currentTurn = Colors::white;
}

Pieces Board::pieceAt(int square) const
{
    bitBoard squareMask = 1ULL << square;

    for (int i = 1; i <= 6; i++)
    {
        if ((gameState.pieceMasks[i] & squareMask) != 0)
        {
            return static_cast<Pieces>(i);
        }
    }

    return Pieces::emptySquare;
}

void Board::printBoard() const {
    char pieceSymbols[] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            
            int square = (rank * 8) + file; 
            char symbolToPrint = ' '; 

            for (int i = 1; i <= 6; i++) { 
                if ((gameState.pieceMasks[i] & (1ULL << square)) != 0) {
                    symbolToPrint = pieceSymbols[i];
                    break;
                }
            }

            // FIXED: Added parentheses around the bitwise AND operation
            if ((gameState.colorMasks[(int)Colors::black] & (1ULL << square)) != 0)
            {
                symbolToPrint = std::tolower(symbolToPrint);
            }
            
            if (symbolToPrint == ' ') {
                printf(". ");
            } else {
                printf("%c ", symbolToPrint);
            }
        }
        printf("\n");
    }
}

void Board::loadFEN(const std::string& fen) {
    clearBoard();

    std::istringstream fenStream(fen);
    std::string piecePlacement, activeColor, castlingRights, enPassant, halfMove, fullMove;

    fenStream >> piecePlacement >> activeColor >> castlingRights >> enPassant >> halfMove >> fullMove;
    int rank = 7;
    int file = 0;

    for(char c : piecePlacement)
    {
        if(c == '/')
        {
            rank--;
            file = 0;
            continue;
        }

        if(std::isdigit(c))
        {
            file += c - '0';
            continue;
        }

        if(std::isalpha(c))
        {
            int square = (rank * 8) + file;
            int colorIndex = std::isupper(c) ? 0 : 1;
            Pieces pieceType = Pieces::emptySquare;

            switch(std::tolower(c))
            {
                case 'p': pieceType = Pieces::pawn; break;
                case 'n': pieceType = Pieces::knight; break;
                case 'b': pieceType = Pieces::bishop; break;
                case 'r': pieceType = Pieces::rook; break;
                case 'q': pieceType = Pieces::queen; break;
                case 'k': pieceType = Pieces::king; break;
                default: break;
            }

            if(pieceType != Pieces::emptySquare)
            {
                gameState.pieceMasks[(int)pieceType] |= (1ULL << square);
                gameState.colorMasks[colorIndex] |= (1ULL << square);
            }
            file++;
            continue;
        }
        
        if(c == ' ')
        {
            break;
        }
    }

    if (activeColor == "w")
    {
    gameState.currentTurn = Colors::white;
    } else 
    {
    gameState.currentTurn = Colors::black;
    };

    for( char c : castlingRights)
    {
        if (c == 'K') gameState.castleWhiteKing = true;
        if (c == 'Q') gameState.castleWhiteQueen = true;
        if(c == 'k') gameState.castleBlackKing = true;
        if(c == 'q') gameState.castleBlackQueen = true;

    };

    if(enPassant == "-")
    {
        gameState.enPassantTargetSquare = 200;
    } else 
    {
        int epFile = enPassant[0] - 'a'; // eg e - a = 4 whic is the correct file

        int epRank = enPassant[1] - '1'; // eg 3 - 1 = 2

        gameState.enPassantTargetSquare = (epRank * 8) + epFile;
    }

    gameState.halfMoveCount = std::stoi(halfMove);
    gameState.fullMoveCount = std:: stoi(fullMove);



}
void Board::addSliderMoves(int square, Pieces piece, const int directions[][2], int directionCount, std::vector<Move>& moves) const
{
    int us = (int)gameState.currentTurn;
    int them = 1 - us;

    int startRank = square / 8;
    int startFile = square % 8;

    for (int d = 0; d < directionCount; d++)
    {
        int rankStep = directions[d][0];
        int fileStep = directions[d][1];

        int rank = startRank + rankStep;
        int file = startFile + fileStep;

        while (rank >= 0 && rank < 8 && file >= 0 && file < 8)
        {
            int target = (rank * 8) + file;
            bitBoard targetMask = 1ULL << target;

            // blocked by our own piece
            if ((gameState.colorMasks[us] & targetMask) != 0)
            {
                break;
            }

            Move move{};
            move.sourceSquare = static_cast<uint8_t>(square);
            move.targetSquare = static_cast<uint8_t>(target);
            move.movedPiece = piece;

            bool isCapture = (gameState.colorMasks[them] & targetMask) != 0;
            if (isCapture)
            {
                move.capturedPiece = pieceAt(target);
            }

            moves.push_back(move);

            // can capture an enemy piece but not slide past it
            if (isCapture)
            {
                break;
            }

            rank += rankStep;
            file += fileStep;
        }
    }
}

void Board::generateSliderMoves(std::vector<Move>& moves) const
{
    bitBoard ourPieces = gameState.colorMasks[(int)gameState.currentTurn];

    bitBoard rooks = gameState.pieceMasks[(int)Pieces::rook] & ourPieces;
    bitBoard bishops = gameState.pieceMasks[(int)Pieces::bishop] & ourPieces;
    bitBoard queens = gameState.pieceMasks[(int)Pieces::queen] & ourPieces;

    // pop the lowest set bit each loop to visit every piece once
    while (rooks != 0)
    {
        int square = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        addSliderMoves(square, Pieces::rook, rookDirections, 4, moves);
    }

    while (bishops != 0)
    {
        int square = __builtin_ctzll(bishops);
        bishops &= bishops - 1;
        addSliderMoves(square, Pieces::bishop, bishopDirections, 4, moves);
    }

    // a queen is a rook and a bishop combined
    while (queens != 0)
    {
        int square = __builtin_ctzll(queens);
        queens &= queens - 1;
        addSliderMoves(square, Pieces::queen, rookDirections, 4, moves);
        addSliderMoves(square, Pieces::queen, bishopDirections, 4, moves);
    }
}

std::string moveToString(const Move& move)
{
    std::string text;
    text += static_cast<char>('a' + move.sourceSquare % 8);
    text += static_cast<char>('1' + move.sourceSquare / 8);
    text += static_cast<char>('a' + move.targetSquare % 8);
    text += static_cast<char>('1' + move.targetSquare / 8);
    return text;
}
