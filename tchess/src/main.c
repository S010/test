#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <curses.h>

static const char *pieces[] = {
	"♚",
	"♛",
	"♜",
	"♝",
	"♞",
	"♟",
	"♔",
	"♕",
	"♖",
	"♗",
	"♘",
	"♙"
};

enum pieces {
	BLACK_KING,
	BLACK_QUEEN,
	BLACK_ROOK,
	BLACK_BISHOP,
	BLACK_KNIGHT,
	BLACK_PAWN,
	WHITE_KING,
	WHITE_QUEEN,
	WHITE_ROOK,
	WHITE_BISHOP,
	WHITE_KNIGHT,
	WHITE_PAWN,
	EMPTY
};

#define BLACK_KING	"♚"
#define BLACK_QUEEN	"♛"
#define BLACK_ROOK	"♜"
#define BLACK_BISHOP	"♝"
#define BLACK_KNIGHT	"♞"
#define BLACK_PAWN	"♟"

#define WHITE_KING	"♔"
#define WHITE_PAWN	"♙"
#define WHITE_QUEEN	"♕"
#define WHITE_ROOK	"♖"
#define WHITE_BISHOP	"♗"
#define WHITE_KNIGHT	"♘"

static char board[8 * 8] = {
	'r', 'k', 'b', 'q', 'k', 'b', 'k', 'r',
	'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
	'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
	'R', 'K', 'B', 'Q', 'K', 'B', 'K', 'R',
};

static const char *
strpiece(char ch)
{
	struct {
		char		 ch;
		const char	*s;
	} tab[] = {
		{ 'k', BLACK_KING },
		{ 'q', BLACK_QUEEN },
		{ 'r', BLACK_ROOK },
		{ 'b', BLACK_BISHOP },
		{ 'k', BLACK_KNIGHT },
		{ 'p', BLACK_PAWN },
		{ 'K', WHITE_KING },
		{ 'Q', WHITE_QUEEN },
		{ 'R', WHITE_ROOK },
		{ 'B', WHITE_BISHOP },
		{ 'K', WHITE_KNIGHT },
		{ 'P', WHITE_PAWN },
		{ ' ', " " },
		{ '\0', NULL },
	}, *tabp;

	for (tabp = tab; tabp->s != NULL; ++tabp) {
		if (tabp->ch == ch)
			return tabp->s;
	}
	return NULL;
}

enum edge_types {
	TOP,
	MIDDLE,
	BOTTOM
};

static void
draw_edge(enum edge_types type)
{
	const int	 n = BOARD_SIZE * 2;
	int		 i;
	const char	*left;
	const char	*right;
	const char	*mid;

	switch (type) {
	case TOP:
		left = "┌";
		right = "┐";
		mid = "┬";
		break;
	case MIDDLE:
		left = "├";
		right = "┤";
		mid = "┼";
		break;
	case BOTTOM:
		left = "└";
		right = "┘";
		mid = "┴";
	}

	printw(left);
	for (i = 0; i < (n - 1); ++i) {
		if (i % 2 == 0)
			printw("─");
		else
			printw(mid);
	}
	printw(right);
}

typedef void (draw_board_func)(struct rect *);

static const char *
format(const char *fmt, ...)
{
	static char	 buf[1024];
	va_list		 ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	return buf;
}

static void
draw_letter_row(int y, int x)
{
	move(y, x);
	for (int i = 0; i < BOARD_SIZE; ++i)
		printw(format("%c ", 'a' + i));
}

static void
draw_board(struct rect *rectp, const char *board)
{
	int	 x;
	int	 y;
	int	 i;
	int	 j;
	int	 row;

	x = rectp->x + 2;
	y = rectp->y;
	rectp->w = 0;
	rectp->h = 0;

	move(y, x);
	draw_letter_row(y++, x + 1);

	move(y++, x);
	draw_edge(TOP);

	for (i = 0; i < BOARD_SIZE; ++i) {
		move(y, x - 2);
		printw(format("%d ", BOARD_SIZE - i));
		row = i * 8;
		move(y++, x);
		for (j = 0; j < BOARD_SIZE; ++j) {
			if (j == 0)
				printw("│");
			printw(strpiece(board[row + j]));
			printw("│");
		}
		printw(format(" %d", BOARD_SIZE - i));
		if (i < BOARD_SIZE - 1) {
			move(y++, x);
			draw_edge(MIDDLE);
		}
	}

	move(y++, x);
	draw_edge(BOTTOM);

	move(y, x);
	draw_letter_row(y++, x + 1);
}

int
main(int argc, char **argv)
{
	struct rect	 rect;

	setlocale(LC_ALL, "en_US.UTF-8");
	initscr();
	rect.x = 0;
	rect.y = 0;
	draw_board(&rect);
	refresh();
	getch();
	endwin();

	return EXIT_SUCCESS;
}
