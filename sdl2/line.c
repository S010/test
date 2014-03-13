#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

struct point {
	int	 x;
	int	 y;
};

const double	 PI = 3.14159265359;
const int	 WINDOW_WIDTH = 800;
const int	 WINDOW_HEIGHT = 600;

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static void
drawline(SDL_Renderer *r, int x0, int y0, int x1, int y1)
{
	int	 dx;
	int	 dy;
	int	 sx;
	int	 sy;
	int	 error;
	int	 error2;

	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	error = dx - dy;

	if (x0 < x1)
		sx = 1;
	else
		sx = -1;

	if (y0 < y1)
		sy = 1;
	else
		sy = -1;

#define PLOT(x, y) \
	SDL_RenderDrawPoint(r, x, y)

	for (;;) {
		PLOT(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		error2 = error * 2;
		if (error2 > -dy) {
			error = error - dy;
			x0 = x0 + sx;
		}
		if (x0 == x1 && y0 == y1) {
			PLOT(x0, y0);
			break;
		}
		if (error2 < dx) {
			error = error + dx;
			y0 = y0 + sy;
		}
	}

#undef PLOT
}

static void
mainloop(struct point *line)
{
	SDL_Event	 event;
	bool		 drawing = false;

	if (line[0].x == -1) {
		line[0].x = 0;
		line[0].y = 0;
		line[1].x = WINDOW_WIDTH - 1;
		line[1].y = WINDOW_HEIGHT - 1;
	}

	for (;;) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_MOUSEBUTTONDOWN:
				line[0].x = event.button.x;
				line[0].y = event.button.y;
				line[1] = line[0];
				drawing = true;
				break;
			case SDL_MOUSEMOTION:
				if (drawing) {
					line[1].x = event.motion.x;
					line[1].y = event.motion.y;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				drawing = false;
				break;
			case SDL_QUIT:
				return;
			}
		}

		SDL_RenderClear(renderer);
		drawline(renderer, line[0].x, line[0].y, line[1].x, line[1].y);
		SDL_RenderPresent(renderer);
		SDL_Delay(1);
	}

	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Line",
	    SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED,
	    WINDOW_WIDTH,
	    WINDOW_HEIGHT,
	    0);
	if (window == NULL)
		errx(EXIT_FAILURE, "SDL_CreateWindow: %s", SDL_GetError());

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == NULL)
		errx(EXIT_FAILURE, "SDL_CreateRenderer: %s", SDL_GetError());
}

static void
cleanup(void)
{
	if (renderer != NULL)
		SDL_DestroyRenderer(renderer);
	if (window != NULL)
		SDL_DestroyWindow(window);
}

int
main(int argc, char **argv)
{
	struct point	 line[2];

	if (argc == 2) {
		sscanf(argv[1],
		    " %d %d %d %d ",
		    &line[0].x,
		    &line[0].y,
		    &line[1].x,
		    &line[1].y);
	} else {
		line[0].x = line[0].y = line[1].x = line[1].y = -1;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		errx(EXIT_FAILURE, "SDL_Init: %s", SDL_GetError());
	atexit(SDL_Quit);
	atexit(cleanup);

	initvideo();
	mainloop(line);

	return EXIT_SUCCESS;
}


