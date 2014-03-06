#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

int
main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		errx(EXIT_FAILURE, "SDL_Init: %s", SDL_GetError());
	atexit(SDL_Quit);

	return EXIT_SUCCESS;
}
