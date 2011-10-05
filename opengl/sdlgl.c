#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <err.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define SCREEN_W 640
#define SCREEN_H 480

static void init_sdl(void);
static void init_opengl(void);
static void main_loop(void);

SDL_Surface *screen;

int
main(int argc, char **argv)
{
	init_sdl();
	init_opengl();
	main_loop();

	return 0;
}

static void
init_sdl(void)
{
	const SDL_VideoInfo *info;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		errx(1, "SDL_Init: %s", SDL_GetError());
	atexit(SDL_Quit);

	info = SDL_GetVideoInfo();
	if (info == NULL)
		errx(1, "SDL_GetVideoInfo: %s", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, info->vfmt->BitsPerPixel,
	    SDL_OPENGL);
	if (screen == NULL)
		errx(1, "SDL_SetVideoMode: %s", SDL_GetError());
}

static void
init_opengl(void)
{
	float ratio;

	glShadeModel(GL_SMOOTH);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glClearColor(0, 0, 0, 0);

	glViewport(0, 0, screen->w, screen->h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ratio = (float) screen->w / (float) screen->h;
	/* TODO replace with call to glFrustum */
	gluPerspective(60.0, ratio, 1.0, 1024.0);
}

static void
main_loop(void)
{
	SDL_Event event;

	for ( ;; ) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit(0);
			}
		}

		/* Insert drawing code here */

		SDL_GL_SwapBuffers();
	}
}
