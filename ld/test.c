#include <unistd.h>
#include <err.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

struct lib {
	const char	*name;
	void		*handle;
};

static void
loadlibs(struct lib *libs)
{
	puts("loading");
	for (; libs->name != NULL; ++libs) {
		libs->handle = dlopen(libs->name, RTLD_NOW);
		if (libs->handle == NULL)
			errx(1, "dlopen: %s: %s", libs->name, dlerror());
	}
}

static void
unloadlibs(struct lib *libs)
{
	puts("unloading");
	for (; libs->name != NULL; ++libs) {
		if (dlclose(libs->handle))
			errx(1, "dlclose: %s: %s", libs->name, dlerror());
	}
}

int
main(int argc, char **argv)
{
	struct lib libs[] = {
		{ "libX11.so" },
		{ "libXcursor.so" },
		{ "libXrandr.so" },
		{ NULL },
	};

	loadlibs(libs);
	unloadlibs(libs);
	loadlibs(libs); /* XXX crash! */

	return 0;
}
