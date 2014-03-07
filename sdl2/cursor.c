#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static void
mainloop(void)
{
	SDL_Event	 event;
	const int	 curssize = 3;
	const int	 moveamount = 10;
	SDL_Rect	 curs = { // "cursor", a rectangle which can be moved with arrow keys
		WINDOW_WIDTH / 2 - curssize / 2, WINDOW_HEIGHT / 2 - curssize / 2,
		curssize, curssize
	};

	while (SDL_WaitEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return;
		case SDL_WINDOWEVENT:
		case SDL_SYSWMEVENT:
			break;
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				curs.y -= moveamount;
				break;
			case SDLK_DOWN:
				curs.y += moveamount;
				break;
			case SDLK_LEFT:
				curs.x -= moveamount;
				break;
			case SDLK_RIGHT:
				curs.x += moveamount;
				break;
			}
			break;
		default:
			continue;
		}
		SDL_RenderClear(renderer);
		SDL_RenderFillRect(renderer, &curs);
		SDL_RenderPresent(renderer);
	}

	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Cursor",
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
