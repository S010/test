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
static void draw(void);

SDL_Surface *screen;
int should_rotate;
Uint32 ticks;

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
	glShadeModel(GL_SMOOTH);

	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glClearColor(0, 0, 0, 0);

	glViewport(0, 0, screen->w, screen->h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	/* TODO replace with call to glFrustum */
	// gluPerspective(60.0, ratio, 1.0, 1024.0);
	gluOrtho2D(-screen->w/2, screen->w/2, -screen->h/2, screen->h/2);
}

static void
main_loop(void)
{
	SDL_Event event;

	ticks = SDL_GetTicks();

	for ( ;; ) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit(0);
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					should_rotate ^= 1;
					break;
				}
				break;
			}
		}

		draw();
	}
}

static void
draw( void )
{
	/* Our angle of rotation. */
	static float angle = 0.0f;
	Uint32 now = SDL_GetTicks();
	double t = now - ticks;

	/*
	 * EXERCISE:
	 * Replace this awful mess with vertex
	 * arrays and a call to glDrawElements.
	 *
	 * EXERCISE:
	 * After completing the above, change
	 * it to use compiled vertex arrays.
	 *
	 * EXERCISE:
	 * Verify my windings are correct here ;).
	 */
	int halfw = screen->w / 2, halfh = screen->h / 2;
	
	static GLint v0[] = { -100, -100 };
	static GLint v1[] = { -100,  100 };
	static GLint v2[] = {  100,  100 };
	static GLint v3[] = {  100, -100};
	static GLubyte red[]	= { 255,   0,   0, 255 };
	static GLubyte green[]  = {   0, 255,   0, 255 };
	static GLubyte blue[]   = {   0,   0, 255, 255 };
	static GLubyte white[]  = { 255, 255, 255, 255 };

	/* Clear the color and depth buffers. */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	/* We don't want to modify the projection matrix. */
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	/* Rotate. */
	glRotatef( angle, 0.0, 0.0, 1.0 );

	if( should_rotate ) {
		angle += 0.1f * t;

		if( angle > 360.0f ) {
			angle = 0.0f;
		}

	}

	/* Send our triangle data to the pipeline. */
	glBegin( GL_POLYGON );

	glColor4ubv( red );
	glVertex2iv( v0 );
	glColor4ubv( green );
	glVertex2iv( v1 );
	glColor4ubv( blue );
	glVertex2iv( v2 );
	glColor4ubv( white );
	glVertex2iv( v3 );

	glEnd( );

	SDL_GL_SwapBuffers();

	if (t > 0.0)
		ticks = now;
}

