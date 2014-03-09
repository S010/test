#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

const double PI = 3.14159265358979323846264338327;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

static SDL_Window	*window;
static SDL_Renderer	*renderer;

static void
rotozoom(SDL_Surface *src, SDL_Surface *dst, double angle, double scale)
{
	int	 Ox; // origin
	int	 Oy;
	int	 x;
	int	 y;
	int	 xp; // x prime
	int	 yp; // y prime
	void	*srcpx;
	void	*dstpx;
	double	 c;
	double	 s;
	int	 bpp; // bytes per pixel
	SDL_Rect	 rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = dst->w;
	rect.h = dst->h;
	SDL_FillRect(dst, &rect, 0);

	c = cos(angle);
	s = sin(angle);

	Ox = src->w / 2;
	Oy = src->h / 2;

	bpp = src->format->BytesPerPixel;

	for (y = 0; y < src->h; ++y) {
		for (x = 0; x < src->w; ++x) {
			srcpx = src->pixels + y * src->w * bpp + x * bpp;
			// rotate around Ox,Oy and scale
			xp = ((double) (x - Ox) * c + (double) (y - Oy) * -s) * scale + Ox;
			yp = ((double) (x - Ox) * s + (double) (y - Oy) * c) * scale + Oy;
			// can't allow writing beyond the pixel array
			if (xp < 0 || yp < 0 || xp >= dst->w || yp >= dst->h)
				continue;
			dstpx = dst->pixels + yp * src->w * bpp + xp * bpp;
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

	origimg = loadimg("spaceship.bmp");
	img = SDL_CreateRGBSurface(
	    0,
	    origimg->w,
	    origimg->h,
	    origimg->format->BitsPerPixel,
	    origimg->format->Rmask,
	    origimg->format->Gmask,
	    origimg->format->Bmask,
	    origimg->format->Amask);

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
		SDL_RenderCopy(renderer, tex, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Rotozoom",
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
