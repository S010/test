#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <complex.h>
#include <math.h>

#include <SDL2/SDL.h>

const double Pi = 3.14159265359;

const int	 WINDOW_WIDTH = 1024;
const int	 WINDOW_HEIGHT = 768;

static SDL_Window	*window;
static SDL_Renderer	*renderer;

struct mandelbrot {
	bool belongs;
	int niter;
	double slope;
};

static const int min_iterations = 150;
static int iterations = min_iterations;

double	 ishift;
double	 jshift;
double	 xrange;
double	 yrange;

static SDL_Color* palette;

int ipow(int x, int n) {
	int result = x;
	while (--n)
		result *= x;
	return result;
}

static void genpalette(void)
{
	SDL_Color	 col;
	const int	 palettesize = iterations;
	double		 l;
	double		 lmax;
	double		 lrange = 10000.0;
	double		 t;
	double		 mul = 200.0;

	lmax = log(1.0 + lrange);

	palette = realloc(palette, sizeof(*palette) * palettesize);
	for (int i = 0; i < palettesize; ++i) {
		l = log(1.0 + ((double) i * (lrange / (double) palettesize)));
		/*
		col.r = 255.0 * (l / lmax);
		col.g = (double) i * (255.0 / (double) palettesize);
		col.b = 255 - col.r;
		*/
		//col.r = col.g = col.b = (double) i * (255.0 / (double) palettesize);
		/*
		t = (sin((double) i * ((Pi*2) / (double) palettesize)) + 1.0) / 2.0;
		col.r = 255.0 * t;
		t = (sin(((double) i * ((Pi*2) / (double) palettesize)) + Pi/3.0) + 1.0) / 2.0;
		col.g = 255.0 * t;
		t = (cos((double) i * ((Pi*2) / (double) palettesize)) + 1.0) / 2.0;
		col.b = 255.0 * t;
		*/
		/*
		t = (sin((double) i * ((Pi) / (double) palettesize)) + 1.0) / 2.0;
		col.r = 255.0 * t;
		t = (sin(((double) i * ((Pi) / (double) palettesize)) + Pi/7.0) + 1.0) / 2.0;
		col.g = 255.0 * t;
		t = (cos((double) i * ((Pi) / (double) palettesize)) + 1.0) / 2.0;
		col.b = 255.0 * t;
		*/
		t = (cos((double) i * ((Pi * mul) / (double) palettesize)) + 1.0) / 2.0;
		col.r = 255.0 * t;
		t = (sin(((double) i * ((Pi * mul) / (double) palettesize)) + Pi/3.0) + 1.0) / 2.0;
		col.g = 255.0 * t;
		t = (sin((double) i * ((Pi * mul) / (double) palettesize)) + 1.0) / 2.0;
		col.b = 255.0 * t;
		col.a = SDL_ALPHA_OPAQUE;
		palette[i] = col;
	}
}

static struct mandelbrot
ismandelbrot(float complex c)
{
	static double		 limit = 0.0;
	float complex		 z;
	int			 i;
	struct mandelbrot	 m;

	if (limit == 0.0)
		limit = sqrt(5);

	m.belongs = true;
	m.slope = 0.0;
	m.niter = 0;
	z = c;
	for (i = 0; i < iterations; ++i) {
		z = z * z + c;
		if (cabsf(z) > limit) {
			m.belongs = false;
			m.niter = i;
			m.slope = (double) i / (double) iterations;
			break;
		}
	}
	return m;
}

static inline double
lerp(double x, double y, double t)
{
	return x + (y - x) * t;
}

static inline SDL_Color
lerp_color(SDL_Color *c1, SDL_Color *c2, double t)
{
	SDL_Color	 c;

	c.r = lerp(c1->r, c2->r, t);
	c.g = lerp(c1->g, c2->g, t);
	c.b = lerp(c1->b, c2->b, t);
	c.a = lerp(c1->a, c2->a, t);

	return c;
}

static void
mainloop(void)
{
	struct mandelbrot	 m;
	SDL_Event		 event;
	int			 Px;
	int			 Py;
	double			 x0;
	double			 y0;
	double			 x;
	double			 y;
	const double		 ratio = (double) WINDOW_WIDTH / (double) WINDOW_HEIGHT;
	const double		 baserange = 3.0;
	float complex		 c;
	double			 step = 0.1;
	bool			 output_params = false;
	unsigned int		 color;
	SDL_Color		 col;

	if (ishift == 0.0) {
		xrange = ratio * baserange;
		yrange = baserange;
	}

	goto start;
	while (SDL_WaitEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return;
		case SDL_WINDOWEVENT:
			if (event.window.type == SDL_WINDOWEVENT_EXPOSED)
				break;
			continue;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				output_params = true;
				ishift -= step;
				break;
			case SDLK_RIGHT:
				output_params = true;
				ishift += step;
				break;
			case SDLK_UP:
				output_params = true;
				jshift -= step;
				break;
			case SDLK_DOWN:
				output_params = true;
				jshift += step;
				break;
			case SDLK_EQUALS:
			case SDLK_PLUS:
				output_params = true;
				xrange /= 2.0;
				yrange /= 2.0;
				step /= 2.0;
				break;
			case SDLK_MINUS:
				output_params = true;
				xrange *= 2.0;
				yrange *= 2.0;
				step *= 2.0;
				break;
			case SDLK_8:
				iterations = min_iterations;
				genpalette();
				printf("%d iterations\n", iterations);
				break;
			case SDLK_0:
				iterations *= 2;
				genpalette();
				printf("%d iterations\n", iterations);
				break;
			case SDLK_9:
				if (iterations > min_iterations) {
					iterations /= 2;
					genpalette();
					printf("%d iterations\n", iterations);
				}
				break;
			}
			if (output_params)
				printf("%lf %lf %lf %lf\n", ishift, jshift, xrange, yrange);
			output_params = false;
			break;
		case SDL_MOUSEBUTTONUP:
			x0 = (double) event.button.x * xrange / (double) WINDOW_WIDTH - (xrange / 2.0) + ishift;
			y0 = (double) event.button.y * yrange / (double) WINDOW_HEIGHT - (yrange / 2.0) + jshift;
			c = x0 + y0*I;
			m = ismandelbrot(c);
			printf("belongs=%s\nniter=%d\n", m.belongs ? "true" : "false", m.niter);
			continue;
		default:
			continue;
		}

start:
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		for (Py = 0; Py < WINDOW_HEIGHT; ++Py) {
			for (Px = 0; Px < WINDOW_WIDTH; ++Px) {
				x0 = (double) Px * xrange / (double) WINDOW_WIDTH - (xrange / 2.0) + ishift;
				y0 = (double) Py * yrange / (double) WINDOW_HEIGHT - (yrange / 2.0) + jshift;

				x = 0.0;
				y = 0.0;

				double iter = 0.0;

				while ((x*x + y*y) < (2 << 16) && iter < iterations) {
					double xtemp = x*x - y*y + x0;
					y = 2*x*y + y0;
					x = xtemp;
					iter = iter + 1;
				}

				if (iter < iterations) {
					double zn = sqrt(x*x + y*y);
					double nu = log(log(zn) / log(2)) / log(2);
					iter = iter + 1 - nu;
				}

				SDL_Color col1 = palette[(int) iter];
				SDL_Color col2 = palette[(int) (iter + 1.0)];
				double rem = fmod(iter, 1.0);
				SDL_Color col = lerp_color(&col1, &col2, rem);

				SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
				SDL_RenderDrawPoint(renderer, Px, Py);

				/*
				c = i + j*I;
				m = ismandelbrot(c);
				if (!m.belongs) {
					col = palette[m.niter];
					SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
					SDL_RenderDrawPoint(renderer, x0, y0);
				}
				*/
			}
		}
		SDL_RenderPresent(renderer);
	}
	errx(EXIT_FAILURE, "SDL_WaitEvent: %s", SDL_GetError());
}

static void
initvideo(void)
{
	window = SDL_CreateWindow(
	    "Mandelbrot Set",
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

	if (argc > 4) {
		int i = 1;
		sscanf(argv[i++], "%lf", &ishift);
		sscanf(argv[i++], "%lf", &jshift);
		sscanf(argv[i++], "%lf", &xrange);
		sscanf(argv[i++], "%lf", &yrange);
	}

	genpalette();
	initvideo();
	mainloop();

	return EXIT_SUCCESS;
}
