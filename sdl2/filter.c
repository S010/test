#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

const double PI = 3.14159265358979323846264338327;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

static SDL_Window	*window;
static SDL_Renderer	*renderer;

// TODO

static double
lerp(double v0, double v1, double t)
{
	return v0 + (v1 - v0) * t;
}

static void
rotozoom(SDL_Surface *src, SDL_Surface *dst, double angle, double scale)
{
	int	 srcOx; // origin
	int	 srcOy;
	int	 dstOx;
	int	 dstOy;
	int	 x;
	int	 y;
	int	 xp; // x prime
	int	 yp; // y prime
	void	*srcpx;
	void	*dstpx;
	double	 c;
	double	 s;
	int	 bpp; // bytes per pixel

	SDL_FillRect(dst, NULL, 0);

	c = cos(-angle);
	s = sin(-angle);

	srcOx = src->w / 2;
	srcOy = src->h / 2;
	dstOx = dst->w / 2;
	dstOy = dst->h / 2;

	if (src->format->BytesPerPixel != dst->format->BytesPerPixel)
		errx(EXIT_FAILURE, "rotozoom: BPP is not the same for src and dst");
	bpp = src->format->BytesPerPixel;

	if (scale == 0.0)
		scale = 0.001;

	for (y = 0; y < dst->h; ++y) {
		for (x = 0; x < dst->w; ++x) {
			dstpx = dst->pixels + y * dst->pitch + x * bpp;
			// rotate around Ox,Oy and scale
			xp = ((double) (x - dstOx) * c + (double) (y - dstOy) * -s) / scale + srcOx;
			yp = ((double) (x - dstOx) * s + (double) (y - dstOy) * c) / scale + srcOy;
			// can't allow reading beyond the pixel array
			if (xp < 0 || yp < 0 || xp >= src->w || yp >= src->h)
				continue;
			srcpx = src->pixels + yp * src->pitch + xp * bpp;
			memcpy(dstpx, srcpx, bpp);
		}
	}
}

SDL_Surface *
loadimg(const char *path)
{
	SDL_Surface	*img;

	img = SDL_LoadBMP(path);
	if (img == NULL)
		errx(EXIT_FAILURE, "SDL_LoadBMP %s: %s", path, SDL_GetError());

	return img;
}

// Create a surface suitable for rotation of s.
SDL_Surface *
createrotosurf(SDL_Surface *s)
{
	SDL_Surface	*srot;
	int		 side;

	side = sqrt(s->w * s->w + s->h * s->h);
	srot = SDL_CreateRGBSurface(
	    0,
	    side,
	    side,
	    s->format->BitsPerPixel,
	    s->format->Rmask,
	    s->format->Gmask,
	    s->format->Bmask,
	    s->format->Amask);
	return srot;
}

static void
mainloop(void)
{
	SDL_Event	 event;
	SDL_Surface	*origimg;
	SDL_Surface	*img;
	SDL_Texture	*tex = NULL;
	double		 scale = 1.0; // scaling factor of the image
	double		 scalestep = 0.1;
	double		 angle = 0.0; // rotation angle of the image in radians
	const double	 anglestep = (2.0*PI) / 36.0; // 10 degrees
	SDL_Rect	 rect;

	origimg = loadimg("spaceship.bmp");
	img = createrotosurf(origimg);

	while (SDL_WaitEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				scale += scalestep;
				break;
			case SDLK_DOWN:
				scale -= scalestep;
				break;
			case SDLK_LEFT:
				angle -= anglestep;
				break;
			case SDLK_RIGHT:
				angle += anglestep;
				break;
			}
			break;
		case SDL_QUIT:
			if (tex != NULL)
				SDL_DestroyTexture(tex);
			return;
		case SDL_WINDOWEVENT:
			break;
		default:
			continue;
		}

		rotozoom(origimg, img, angle, scale);
		if (tex != NULL)
			SDL_DestroyTexture(tex);
		tex = SDL_CreateTextureFromSurface(renderer, img);

		SDL_RenderClear(renderer);
		if (SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h) != 0)
			errx(EXIT_FAILURE, "SDL_QueryTexture: %s", SDL_GetError());
		rect.x = (WINDOW_WIDTH - rect.w) / 2;
		rect.y = (WINDOW_HEIGHT - rect.h) / 2;
		SDL_RenderCopy(renderer, tex, NULL, &rect);
		SDL_RenderPresent(renderer);
	}
	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Filter",
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
	puts("Use arrow keys to manipulate the image...");
	mainloop();

	return EXIT_SUCCESS;
}
