#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

static SDL_Window	*window;
static SDL_Renderer	*renderer;
static SDL_Texture	*texture;

static void
mainloop(void)
{
	SDL_Event	 event;

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
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
loadimage()
{
	SDL_Surface	*image;

	image = SDL_LoadBMP("hello.bmp");
	if (image == NULL)
		errx(EXIT_FAILURE, "SDL_LoadBMP: %s", SDL_GetError());

	texture = SDL_CreateTextureFromSurface(renderer, image);
	if (texture == NULL)
		errx(EXIT_FAILURE, "SDL_CreateTextureFromSurface: %s", SDL_GetError());
	SDL_FreeSurface(image);
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Hello",
	    SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED,
	    640,
	    480,
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
	if (texture != NULL)
		SDL_DestroyTexture(texture);
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
	loadimage();
	mainloop();

	return EXIT_SUCCESS;
}
