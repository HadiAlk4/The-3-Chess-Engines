#include <stdio.h>
#include "Board.h"
#include <cctype>

Board::Board() { 
    for (int i = 0; i < 7; i++) { // i dont think we need this i refactored them in in Types.h well see after some testing
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
            
            int square = (rank * 8) + file; // 1D conversion e.g. when checking for top left (rank 7 file 0) 7 * 8 + 0 = 56th bit
            char symbolToPrint = ' '; 

            for (int i = 1; i <= 6; i++) { // iterate pieces and skip emptySquare
                if ((gameState.pieceMasks[i] & (1ULL << square)) != 0) {
                    symbolToPrint = pieceSymbols[i];
                    break;
                }
            }

            if(gameState.colorMasks[(int)Colors::black] & ((1ULL << square) != 0))
            {
                symbolToPrint = std::tolower(symbolToPrint);
            };
			
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
    
}