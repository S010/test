#ifndef TCHESS_H
#define TCHESS_H

#define BOARD_SIZE	8

struct rect {
	int	 x;
	int	 y;
	int	 w;
	int	 h;
};

enum pieces {
	NONE,
	WHITE_PAWN,
	WHITE_ROOK,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN,
	BLACK_ROOK,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_QUEEN,
	BLACK_KING,
};

#endif
