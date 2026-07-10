#include <stdio.h>
#include "Board.h"


Board::Board() {
    for (int i = 0; i < 7; i++) {
        gameState.pieceMasks[i] = 0;
    }	
	  
    gameState.colorMasks[0] = 0;
    gameState.colorMasks[1] = 0;
    
	gameState.currentTurn = Colors::white;

}

void Board::printBoard() const {
	for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
			printf(". ");
		}
        printf("\n");
    }		
	
}

void Board::loadFEN(const std::string& fen) {
    
}