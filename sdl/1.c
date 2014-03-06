#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

SDL_Window	*window;

static void
sdlerror(const char *what)
{
	errx(1, "%s: %s", what, SDL_GetError());
}

static void
init(void)
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) == -1)
		sdlerror("SDL_Init");
	window = SDL_CreateWindow(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	if (window == NULL)
		sdlerror("SDL_CreateWindow");
	atexit(SDL_Quit);
}

int
main(int argc, char **argv)
{
	init();
	return 0;
}
