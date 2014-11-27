#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <gl.h>

struct point {
	int	 x;
	int	 y;
};

const int	 WINDOW_WIDTH = 800;
const int	 WINDOW_HEIGHT = 600;

static SDL_Window	*window;
static SDL_GLContext	 glcontext;

static void
mainloop(struct point *line)
{
	SDL_Event	 event;
	bool		 drawing = false;
	float		 currentTime;

	if (line[0].x == -1) {
		line[0].x = 0;
		line[0].y = 0;
		line[1].x = WINDOW_WIDTH - 1;
		line[1].y = WINDOW_HEIGHT - 1;
	}

	while (SDL_GetTicks() == 0)
		/* wait */;

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

		currentTime = (float) SDL_GetTicks() / 1000.0;

		GLfloat color[] = { sin(currentTime) * 0.5 + 0.5, cos(currentTime) * 0.5 + 0.5, 0.0, 1.0 };
		glClearBufferfv(GL_COLOR, 0, color);

		SDL_GL_SwapWindow(window);

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
	    SDL_WINDOW_OPENGL);
	if (window == NULL)
		errx(EXIT_FAILURE, "SDL_CreateWindow: %s", SDL_GetError());

	glcontext = SDL_GL_CreateContext(window);
	if (glcontext == NULL)
		errx(EXIT_FAILURE, "SDL_GL_CreateContext: %s", SDL_GetError());
}

static void
cleanup(void)
{
	if (glcontext != NULL)
		SDL_GL_DeleteContext(glcontext);
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

