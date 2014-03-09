#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static const double gconstant = 9.8; // free fall acceleration in m/s^2

struct point {
	int	 x;
	int	 y;
};

struct surface {
	struct point	*pts;
	int		 npts;
};

static SDL_Window	*window;
static SDL_Renderer	*renderer;
static struct surface	 surface;

static void *
xrealloc(void *p, size_t size)
{
	void	*q;

	q = realloc(p, size);
	if (q == NULL)
		err(EXIT_FAILURE, "realloc");
	return q;
}

static void
mainloop(void)
{
	SDL_Event	 event;
	struct point	*pt;

	for (;;) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			}
		}

		SDL_RenderClear(renderer);
		for (pt = surface.pts; pt < (surface.pts + surface.npts - 1); ++pt)
			SDL_RenderDrawLine(renderer, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
		SDL_RenderPresent(renderer);
		SDL_Delay(1);
	}

	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Surface",
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

static void
loadsurface(FILE *fp, struct surface *s)
{
	int		 n;
	struct point	*pts;

	if (fscanf(fp, " %d ", &n) != 1)
		errx(EXIT_FAILURE, "failed to read the number of points");
	if (n < 2)
		errx(EXIT_FAILURE, "invalid number of points specified");
	pts = xrealloc(NULL, n * sizeof(*pts));
	s->npts = n;
	s->pts = pts;
	while (n--) {
		if (fscanf(fp, " %d %d ", &pts->x, &pts->y) != 2)
			errx(EXIT_FAILURE, "failed to read points");
		pts->y = WINDOW_HEIGHT - pts->y;
		++pts;
	}
}

int
main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
		errx(EXIT_FAILURE, "SDL_Init: %s", SDL_GetError());
	atexit(SDL_Quit);
	atexit(cleanup);

	memset(&surface, 0, sizeof surface);
	puts("reading surface points from stdin...");
	loadsurface(stdin, &surface);
	puts("done.");

	initvideo();
	mainloop();

	return EXIT_SUCCESS;
}
