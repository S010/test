#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <complex.h>

#include <SDL2/SDL.h>

const int	 WINDOW_WIDTH = 800;
const int	 WINDOW_HEIGHT = 600;

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static bool
ismandelbrot(float complex c)
{
	float complex	 znext;
	float complex	 z;
	int		 niter = 1000;

	znext = z = c;
	while (niter--) {
		znext = z * z + c;
		if (cabsf(znext) > 2.0)
			return true;
		z = znext;
	}
	return false;
}

static void
mainloop(void)
{
	SDL_Event	 event;
	int		 x;
	int		 y;
	float		 i;
	float		 j;
	float		 range = 2.0;
	float		 rangestep = 0.4;
	float complex	 c;
	const float	 step = 0.2;
	float		 ishift = 0.0;
	float		 jshift = 0.0;

	while (SDL_WaitEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return;
		case SDL_WINDOWEVENT:
			if (event.window.type == SDL_WINDOWEVENT_EXPOSED)
				break;
			continue;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				ishift += step;
				break;
			case SDLK_RIGHT:
				ishift -= step;
				break;
			case SDLK_UP:
				jshift -= step;
				break;
			case SDLK_DOWN:
				jshift += step;
				break;
			case SDLK_EQUALS:
			case SDLK_PLUS:
				range -= rangestep;
				break;
			case SDLK_MINUS:
				range += rangestep;
				break;
			}
			break;
		default:
			continue;
		}

		SDL_RenderClear(renderer);
		for (y = 0; y < WINDOW_HEIGHT; ++y) {
			for (x = 0; x < WINDOW_WIDTH; ++x) {
				i = (float) x * range / (float) WINDOW_WIDTH - (range / 2.0) + ishift;
				j = (float) y * range / (float) WINDOW_HEIGHT - (range / 2.0) + jshift;
				c = i + j*I;
				if (ismandelbrot(c))
					SDL_RenderDrawPoint(renderer, x, y);
			}
		}
		SDL_RenderPresent(renderer);
	}
	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Mandelbrot Set",
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
