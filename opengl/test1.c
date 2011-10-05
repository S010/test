#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <err.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define DEFAULT_MODEL_DISTANCE -5.0f

static void init_sdl(void);
static void init_opengl(void);
static void main_loop(void);
static void draw(double);

SDL_Surface *screen;
int should_rotate;
float model_distance = DEFAULT_MODEL_DISTANCE;
Uint32 prev_ticks;


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
	Uint32 ticks;
	double dt;


	prev_ticks = SDL_GetTicks();

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
				case SDLK_HOME:
					model_distance = DEFAULT_MODEL_DISTANCE;
					break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
				case 4:
					model_distance += 1.0f;
					break;
				case 5:
					model_distance -= 1.0f;
					break;
				}
				break;
			}
		}

		ticks = SDL_GetTicks();
		dt = ticks - prev_ticks;
		draw(dt);
		if (dt > 0.0)
			prev_ticks = ticks;
	}
}

static void
draw(double dt)
{
	/* Our angle of rotation. */
	static float angle = 0.0f;


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
	static GLfloat v0[] = { -1.0f, -1.0f,  1.0f };
	static GLfloat v1[] = {  1.0f, -1.0f,  1.0f };
	static GLfloat v2[] = {  1.0f,  1.0f,  1.0f };
	static GLfloat v3[] = { -1.0f,  1.0f,  1.0f };
	static GLfloat v4[] = { -1.0f, -1.0f, -1.0f };
	static GLfloat v5[] = {  1.0f, -1.0f, -1.0f };
	static GLfloat v6[] = {  1.0f,  1.0f, -1.0f };
	static GLfloat v7[] = { -1.0f,  1.0f, -1.0f };
	static GLubyte red[]	= { 255,   0,   0, 255 };
	static GLubyte green[]  = {   0, 255,   0, 255 };
	static GLubyte blue[]   = {   0,   0, 255, 255 };
	static GLubyte white[]  = { 255, 255, 255, 255 };
	static GLubyte yellow[] = {   0, 255, 255, 255 };
	static GLubyte black[]  = {   0,   0,   0, 255 };
	static GLubyte orange[] = { 255, 255,   0, 255 };
	static GLubyte purple[] = { 255,   0, 255,   0 };

	/* Clear the color and depth buffers. */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	/* We don't want to modify the projection matrix. */
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	/* Move down the z-axis. */
	glTranslatef( 0.0, 0.0, model_distance );

	/* Rotate. */
	glRotatef( angle, 0.0, 1.0, 0.0 );

	if( should_rotate ) {
		angle += 0.1f * dt;

		if( angle > 360.0f ) {
			angle = 0.0f;
		}

	}

	/* Send our triangle data to the pipeline. */
	glBegin( GL_TRIANGLES );

	glColor4ubv( red );
	glVertex3fv( v0 );
	glColor4ubv( green );
	glVertex3fv( v1 );
	glColor4ubv( blue );
	glVertex3fv( v2 );

	glColor4ubv( red );
	glVertex3fv( v0 );
	glColor4ubv( blue );
	glVertex3fv( v2 );
	glColor4ubv( white );
	glVertex3fv( v3 );

	glColor4ubv( green );
	glVertex3fv( v1 );
	glColor4ubv( black );
	glVertex3fv( v5 );
	glColor4ubv( orange );
	glVertex3fv( v6 );

	glColor4ubv( green );
	glVertex3fv( v1 );
	glColor4ubv( orange );
	glVertex3fv( v6 );
	glColor4ubv( blue );
	glVertex3fv( v2 );

	glColor4ubv( black );
	glVertex3fv( v5 );
	glColor4ubv( yellow );
	glVertex3fv( v4 );
	glColor4ubv( purple );
	glVertex3fv( v7 );

	glColor4ubv( black );
	glVertex3fv( v5 );
	glColor4ubv( purple );
	glVertex3fv( v7 );
	glColor4ubv( orange );
	glVertex3fv( v6 );

	glColor4ubv( yellow );
	glVertex3fv( v4 );
	glColor4ubv( red );
	glVertex3fv( v0 );
	glColor4ubv( white );
	glVertex3fv( v3 );

	glColor4ubv( yellow );
	glVertex3fv( v4 );
	glColor4ubv( white );
	glVertex3fv( v3 );
	glColor4ubv( purple );
	glVertex3fv( v7 );

	glColor4ubv( white );
	glVertex3fv( v3 );
	glColor4ubv( blue );
	glVertex3fv( v2 );
	glColor4ubv( orange );
	glVertex3fv( v6 );

	glColor4ubv( white );
	glVertex3fv( v3 );
	glColor4ubv( orange );
	glVertex3fv( v6 );
	glColor4ubv( purple );
	glVertex3fv( v7 );

	glColor4ubv( green );
	glVertex3fv( v1 );
	glColor4ubv( red );
	glVertex3fv( v0 );
	glColor4ubv( yellow );
	glVertex3fv( v4 );

	glColor4ubv( green );
	glVertex3fv( v1 );
	glColor4ubv( yellow );
	glVertex3fv( v4 );
	glColor4ubv( white );
	glVertex3fv( v5 );

	glEnd( );

	SDL_GL_SwapBuffers();
}

