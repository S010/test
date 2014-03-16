#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <complex.h>

#include <SDL2/SDL.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

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
	float complex	 c;

	while (SDL_WaitEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return;
		case SDL_WINDOWEVENT:
		case SDL_SYSWMEVENT:
			break;
		default:
			continue;
		}

		SDL_RenderClear(renderer);
		for (y = 0; y < WINDOW_HEIGHT; ++y) {
			for (x = 0; x < WINDOW_WIDTH; ++x) {
				i = (float) x * 2.0 / (float) WINDOW_WIDTH - 1.0;
				j = (float) y * 2.0 / (float) WINDOW_HEIGHT - 1.0;
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
	    "Hello",
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

#ifdef __STDC_IEC_559_COMPLEX__
	printf("_STDC_IEC_559_COMPLEX__ defined\n");
#else
	printf("_STDC_IEC_559_COMPLEX__ not defined\n");
#endif
#ifdef __STDC_NO_COMPLEX__
	printf("__STDC_NO_COMPLEX__ defined\n");
#else
	printf("__STDC_NO_COMPLEX__ not defined\n");
#endif

	initvideo();
	mainloop();

	return EXIT_SUCCESS;
}
