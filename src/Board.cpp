#include <stdio.h>
#include "Board.h"
#include <cctype>

Board::Board() { 
    // Kept for now: if refactored in Types.h can be removed
    for (int i = 0; i < 7; i++) {
        gameState.pieceMasks[i] = 0;
    }    
      
    gameState.colorMasks[0] = 0;
    gameState.colorMasks[1] = 0;
    
    gameState.currentTurn = Colors::white;
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
    int rank = 7;
    int file = 0;

    for(char c : fen)
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
}