#include <stdio.h>
#include "Board.h"
#include <cctype>
#include <sstream>

// {rankStep, fileStep}
const int rookDirections[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
const int bishopDirections[4][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
const int knightOffsets[8][2] = { {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2} };
const int kingOffsets[8][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
const Pieces promotionPieces[4] = { Pieces::queen, Pieces::rook, Pieces::bishop, Pieces::knight };

// a pawn reaching the last rank becomes four moves, one per promotion piece
static void addPawnMove(Move move, std::vector<Move>& moves)
{
    int targetRank = move.targetSquare / 8;
    if (targetRank != 0 && targetRank != 7)
    {
        moves.push_back(move);
        return;
    }

    for (Pieces piece : promotionPieces)
    {
        move.promotionPiece = piece;
        moves.push_back(move);
    }
}

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

void Board::addLeaperMoves(int square, Pieces piece, const int offsets[][2], int offsetCount, std::vector<Move>& moves) const
{
    int us = (int)gameState.currentTurn;
    int them = 1 - us;

    int startRank = square / 8;
    int startFile = square % 8;

    for (int o = 0; o < offsetCount; o++)
    {
        int rank = startRank + offsets[o][0];
        int file = startFile + offsets[o][1];

        // jump lands off the board
        if (rank < 0 || rank >= 8 || file < 0 || file >= 8)
        {
            continue;
        }

        int target = (rank * 8) + file;
        bitBoard targetMask = 1ULL << target;

        // landing on our own piece
        if ((gameState.colorMasks[us] & targetMask) != 0)
        {
            continue;
        }

        Move move{};
        move.sourceSquare = static_cast<uint8_t>(square);
        move.targetSquare = static_cast<uint8_t>(target);
        move.movedPiece = piece;

        if ((gameState.colorMasks[them] & targetMask) != 0)
        {
            move.capturedPiece = pieceAt(target);
        }

        moves.push_back(move);
    }
}

void Board::generateLeaperMoves(std::vector<Move>& moves) const
{
    bitBoard ourPieces = gameState.colorMasks[(int)gameState.currentTurn];

    bitBoard knights = gameState.pieceMasks[(int)Pieces::knight] & ourPieces;
    bitBoard kings = gameState.pieceMasks[(int)Pieces::king] & ourPieces;

    while (knights != 0)
    {
        int square = __builtin_ctzll(knights);
        knights &= knights - 1;
        addLeaperMoves(square, Pieces::knight, knightOffsets, 8, moves);
    }

    // castling is handled separately with the special rules
    while (kings != 0)
    {
        int square = __builtin_ctzll(kings);
        kings &= kings - 1;
        addLeaperMoves(square, Pieces::king, kingOffsets, 8, moves);
    }
}

void Board::generatePawnMoves(std::vector<Move>& moves) const
{
    int us = (int)gameState.currentTurn;
    int them = 1 - us;

    bitBoard occupied = gameState.colorMasks[0] | gameState.colorMasks[1];
    bitBoard pawns = gameState.pieceMasks[(int)Pieces::pawn] & gameState.colorMasks[us];

    // white pawns move up the board, black pawns move down
    int forward = (gameState.currentTurn == Colors::white) ? 1 : -1;
    int doublePushRank = (gameState.currentTurn == Colors::white) ? 1 : 6;

    while (pawns != 0)
    {
        int square = __builtin_ctzll(pawns);
        pawns &= pawns - 1;

        int startRank = square / 8;
        int startFile = square % 8;
        int rank = startRank + forward;

        // a pawn can never stand on the last rank, it promotes on arrival
        if (rank < 0 || rank >= 8)
        {
            continue;
        }

        Move move{};
        move.sourceSquare = static_cast<uint8_t>(square);
        move.movedPiece = Pieces::pawn;

        // single push: the square ahead must be empty
        int oneAhead = (rank * 8) + startFile;
        if ((occupied & (1ULL << oneAhead)) == 0)
        {
            move.targetSquare = static_cast<uint8_t>(oneAhead);
            addPawnMove(move, moves);

            // double push: only from the starting rank, and both squares must be empty
            int twoAhead = oneAhead + (forward * 8);
            if (startRank == doublePushRank && (occupied & (1ULL << twoAhead)) == 0)
            {
                move.targetSquare = static_cast<uint8_t>(twoAhead);
                moves.push_back(move);
            }
        }

        // diagonal captures: only onto an enemy piece
        for (int fileStep = -1; fileStep <= 1; fileStep += 2)
        {
            int file = startFile + fileStep;
            if (file < 0 || file >= 8)
            {
                continue;
            }

            int target = (rank * 8) + file;
            if ((gameState.colorMasks[them] & (1ULL << target)) != 0)
            {
                Move capture = move;
                capture.targetSquare = static_cast<uint8_t>(target);
                capture.capturedPiece = pieceAt(target);
                addPawnMove(capture, moves);
            }
            // en passant: the target square is empty and the captured pawn sits just behind it
            else if (target == gameState.enPassantTargetSquare)
            {
                int capturedSquare = target - (forward * 8);
                bitBoard enemyPawns = gameState.pieceMasks[(int)Pieces::pawn] & gameState.colorMasks[them];

                // guard against a FEN whose en passant square has no pawn to take
                if ((enemyPawns & (1ULL << capturedSquare)) != 0)
                {
                    Move capture = move;
                    capture.targetSquare = static_cast<uint8_t>(target);
                    capture.capturedPiece = Pieces::pawn;
                    capture.wasEnPassant = true;
                    moves.push_back(capture);
                }
            }
        }
    }
}

bool Board::isSquareAttacked(int square, Colors attackerColor) const
{
    bitBoard attackers = gameState.colorMasks[(int)attackerColor];
    bitBoard occupied = gameState.colorMasks[0] | gameState.colorMasks[1];

    bitBoard pawns = gameState.pieceMasks[(int)Pieces::pawn] & attackers;
    bitBoard knights = gameState.pieceMasks[(int)Pieces::knight] & attackers;
    bitBoard kings = gameState.pieceMasks[(int)Pieces::king] & attackers;
    // queens attack like both rooks and bishops
    bitBoard straightSliders = (gameState.pieceMasks[(int)Pieces::rook] | gameState.pieceMasks[(int)Pieces::queen]) & attackers;
    bitBoard diagonalSliders = (gameState.pieceMasks[(int)Pieces::bishop] | gameState.pieceMasks[(int)Pieces::queen]) & attackers;

    int startRank = square / 8;
    int startFile = square % 8;

    // look outward from the square: an attacker on the other end of a pattern hits it
    // white pawns attack upward, so they sit one rank below the square they attack
    int pawnRank = startRank + ((attackerColor == Colors::white) ? -1 : 1);
    if (pawnRank >= 0 && pawnRank < 8)
    {
        for (int fileStep = -1; fileStep <= 1; fileStep += 2)
        {
            int file = startFile + fileStep;
            if (file >= 0 && file < 8 && (pawns & (1ULL << ((pawnRank * 8) + file))) != 0)
            {
                return true;
            }
        }
    }

    for (int o = 0; o < 8; o++)
    {
        int rank = startRank + knightOffsets[o][0];
        int file = startFile + knightOffsets[o][1];
        if (rank >= 0 && rank < 8 && file >= 0 && file < 8 && (knights & (1ULL << ((rank * 8) + file))) != 0)
        {
            return true;
        }

        rank = startRank + kingOffsets[o][0];
        file = startFile + kingOffsets[o][1];
        if (rank >= 0 && rank < 8 && file >= 0 && file < 8 && (kings & (1ULL << ((rank * 8) + file))) != 0)
        {
            return true;
        }
    }

    for (int d = 0; d < 4; d++)
    {
        for (int pass = 0; pass < 2; pass++)
        {
            const int* direction = (pass == 0) ? rookDirections[d] : bishopDirections[d];
            bitBoard sliders = (pass == 0) ? straightSliders : diagonalSliders;

            int rank = startRank + direction[0];
            int file = startFile + direction[1];

            // only the first piece on each ray can attack, anything behind it is blocked
            while (rank >= 0 && rank < 8 && file >= 0 && file < 8)
            {
                bitBoard targetMask = 1ULL << ((rank * 8) + file);
                if ((occupied & targetMask) != 0)
                {
                    if ((sliders & targetMask) != 0)
                    {
                        return true;
                    }
                    break;
                }

                rank += direction[0];
                file += direction[1];
            }
        }
    }

    return false;
}

void Board::generateCastlingMoves(std::vector<Move>& moves) const
{
    bool isWhite = gameState.currentTurn == Colors::white;
    Colors enemy = isWhite ? Colors::black : Colors::white;

    bool kingSideRight = isWhite ? gameState.castleWhiteKing : gameState.castleBlackKing;
    bool queenSideRight = isWhite ? gameState.castleWhiteQueen : gameState.castleBlackQueen;
    if (!kingSideRight && !queenSideRight)
    {
        return;
    }

    bitBoard ourPieces = gameState.colorMasks[(int)gameState.currentTurn];
    bitBoard occupied = gameState.colorMasks[0] | gameState.colorMasks[1];
    bitBoard ourRooks = gameState.pieceMasks[(int)Pieces::rook] & ourPieces;
    bitBoard ourKings = gameState.pieceMasks[(int)Pieces::king] & ourPieces;

    // the rights only say the king and rook haven't moved, so still check they're on their home squares
    int kingSquare = isWhite ? 4 : 60; // e1 or e8
    if ((ourKings & (1ULL << kingSquare)) == 0)
    {
        return;
    }

    // can't castle out of check
    if (isSquareAttacked(kingSquare, enemy))
    {
        return;
    }

    Move move{};
    move.sourceSquare = static_cast<uint8_t>(kingSquare);
    move.movedPiece = Pieces::king;
    move.wasCastling = true;

    // king side: f and g must be empty and safe
    int fSquare = kingSquare + 1;
    int gSquare = kingSquare + 2;
    int hSquare = kingSquare + 3;
    if (kingSideRight
        && (ourRooks & (1ULL << hSquare)) != 0
        && (occupied & ((1ULL << fSquare) | (1ULL << gSquare))) == 0
        && !isSquareAttacked(fSquare, enemy)
        && !isSquareAttacked(gSquare, enemy))
    {
        move.targetSquare = static_cast<uint8_t>(gSquare);
        moves.push_back(move);
    }

    // queen side: d, c and b must be empty, but only d and c need to be safe
    // since the king never touches b
    int dSquare = kingSquare - 1;
    int cSquare = kingSquare - 2;
    int bSquare = kingSquare - 3;
    int aSquare = kingSquare - 4;
    if (queenSideRight
        && (ourRooks & (1ULL << aSquare)) != 0
        && (occupied & ((1ULL << dSquare) | (1ULL << cSquare) | (1ULL << bSquare))) == 0
        && !isSquareAttacked(dSquare, enemy)
        && !isSquareAttacked(cSquare, enemy))
    {
        move.targetSquare = static_cast<uint8_t>(cSquare);
        moves.push_back(move);
    }
}

std::string moveToString(const Move& move)
{
    std::string text;
    text += static_cast<char>('a' + move.sourceSquare % 8);
    text += static_cast<char>('1' + move.sourceSquare / 8);
    text += static_cast<char>('a' + move.targetSquare % 8);
    text += static_cast<char>('1' + move.targetSquare / 8);

    // UCI adds the promotion piece in lowercase, e.g. "e7e8q"
    switch (move.promotionPiece)
    {
        case Pieces::queen: text += 'q'; break;
        case Pieces::rook: text += 'r'; break;
        case Pieces::bishop: text += 'b'; break;
        case Pieces::knight: text += 'n'; break;
        default: break;
    }

    return text;
}
