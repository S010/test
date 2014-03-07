#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static const double gconstant = 9.8; // free fall acceleration in m/s^2

struct point {
	int	 x;
	int	 y;
};

struct vec {
	double	 x;
	double	 y;
};

struct ball {
	int		 x;
	int		 y;
	int		 r;
	struct vec	 vec; // motion vector
};

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static void
move(int *x, int *y, struct vec *v, Uint32 ticks)
{
	v->y += (double) ticks * (gconstant / 1000.0);

	*x += v->x;
	if (*x < 0.0) {
		*x = 0.0;
		v->x = -v->x / 2.0;
	} else if (*x > (double) WINDOW_WIDTH) {
		*x = (double) WINDOW_WIDTH;
		v->x = -v->x / 2.0;
	}

	*y += v->y;
	if (*y < 0.0) {
		*y = 0.0;
		v->y = -v->y / 2.0;
	} else if (*y > WINDOW_HEIGHT) {
		*y = (double) WINDOW_HEIGHT;
		v->y = -v->y / 2.0;
	}
}

static void
mainloop(void)
{
	SDL_Event	 event;
	Uint32		 clock[2];
	Uint32		 ticks;
	struct vec	 throw = { -1.0, -1.0 }; // ball throw vector
	const double	 throwdiv = 10.0;
	struct ball	 ball = { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 8, { 0, 0 } };
	SDL_Rect	 rc;
	struct point	 mousepos;

	clock[0] = SDL_GetTicks();
	for (;;) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_MOUSEBUTTONDOWN:
				throw.x = event.button.x;
				throw.y = event.button.y;
				break;
			case SDL_MOUSEBUTTONUP:
				ball.x = throw.x;
				ball.y = throw.y;
				throw.x = event.button.x - throw.x;
				throw.y = event.button.y - throw.y;
				throw.x /= throwdiv; // scale down a bit...
				throw.y /= throwdiv;
				ball.vec = throw;
				throw.x = -1.0;
				throw.y = -1.0;
				break;
			case SDL_MOUSEMOTION:
				mousepos.x = event.motion.x;
				mousepos.y = event.motion.y;
				break;
			}
		}

		clock[1] = SDL_GetTicks();
		ticks = clock[1] - clock[0];
		if (ticks > 10) {
			clock[0] = clock[1];
			move(&ball.x, &ball.y, &ball.vec, ticks);
		}

		SDL_RenderClear(renderer);

		rc.x = ball.x - ball.r;
		rc.y = ball.y - ball.r;
		rc.w = ball.r * 2;
		rc.h = ball.r * 2;
		SDL_RenderFillRect(renderer, &rc);

		if (throw.x > 0.0) // user is drawing a new throw vector for the ball
			SDL_RenderDrawLine(renderer,
			    (int) throw.x, (int) throw.y, mousepos.x, mousepos.y);

		SDL_RenderPresent(renderer);
	}

	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Free fall",
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
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		errx(EXIT_FAILURE, "SDL_Init: %s", SDL_GetError());
	atexit(SDL_Quit);
	atexit(cleanup);

	initvideo();
	mainloop();

	return EXIT_SUCCESS;
}
