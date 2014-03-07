#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

struct vec {
	int	 x;
	int	 y;
};

struct cursor {
	struct vec	 vec; // vector of motion
	SDL_Rect	 rect; // current pos and size
};

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static void
applyvec(SDL_Rect *r, struct vec *v, int ticks)
{
	r->x += v->x * ticks;
	if (r->x < 0) {
		r->x = 0;
		v->x = -v->x / 2;
	} else if (r->x > WINDOW_WIDTH) {
		r->x = WINDOW_WIDTH;
		v->x = -v->x / 2;
	}

	r->y += v->y * ticks;
	if (r->y < 0) {
		r->y = 0;
		v->y = -v->y / 2;
	} else if (r->y > WINDOW_HEIGHT) {
		r->y = WINDOW_HEIGHT;
		v->y = -v->y / 2;
	}
}

static void
mainloop(void)
{
	const int	 curssize = 3;
	const int	 moveamount = 1;

	struct cursor	 cursor = {
		{ 0, 0 },
		{
			WINDOW_WIDTH / 2 - curssize / 2, WINDOW_HEIGHT / 2 - curssize / 2,
			curssize, curssize
		}
	};
	SDL_Event	 event;
	Uint32		 clock[2];
	Uint32		 ticks;

	clock[0] = SDL_GetTicks();
	for (;;) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					cursor.vec.y -= moveamount;
					break;
				case SDLK_DOWN:
					cursor.vec.y += moveamount;
					break;
				case SDLK_LEFT:
					cursor.vec.x -= moveamount;
					break;
				case SDLK_RIGHT:
					cursor.vec.x += moveamount;
					break;
				}
				break;
			}
		}

		clock[1] = SDL_GetTicks();
		ticks = clock[1] - clock[0];
		if (ticks > 10) {
			clock[0] = clock[1];
			applyvec(&cursor.rect, &cursor.vec, 1);
		}

		SDL_RenderClear(renderer);
		SDL_RenderFillRect(renderer, &cursor.rect);
		SDL_RenderPresent(renderer);
	}

	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Inertia",
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
