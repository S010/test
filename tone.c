#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <unistd.h>
#include <err.h>

#include <ao/ao.h>

static int
get_ao_driver_id()
{
	int	 i;

	i = ao_driver_id("pulse");
	if (i != -1)
		return i;

	i = ao_driver_id("alsa");
	if (i != -1)
		return i;

	i = ao_default_driver_id();
	if (i != -1)
		return i;

	errx(1, "ao_default_driver_id");

	return -1;
}

int
main(int argc, char **argv)
{
	int			 id, quit;
	ao_device		*dev;
	ao_sample_format	 sf;
	short			*buf;
	size_t			 bufsize;
	int			 i, freq;
	double			 a, t;

	ao_initialize();

	id = get_ao_driver_id();

	bzero(&sf, sizeof sf);
	sf.bits = 16;
	sf.rate = 44100;
	sf.channels = 1;
	sf.byte_format = AO_FMT_NATIVE;
	sf.matrix = NULL;

	dev = ao_open_live(id, &sf, NULL);
	if (dev == NULL)
		errx(1, "ao_open_live");

	/* generate 1 second of sine wave sound */
	bufsize = sf.rate * sf.channels;
	buf = malloc(bufsize * sizeof(short));
	if (buf == NULL)
		err(1, "malloc");
	freq = 400;
	a = 0.0;
	t = ((M_PI*2) * (double) freq) / sf.rate;
	for (i = 0; i < sf.rate; ++i) {
		buf[i] = SHRT_MAX * sin(a);
		a += t;
		while (a >= M_PI * 2)
			a -= M_PI * 2;
	}

	for ( ;; ) {
		if (ao_play(dev, buf, bufsize) == -1)
			errx(1, "ao_play");
	}

	ao_close(dev);

	ao_shutdown();

	return 0;
}
