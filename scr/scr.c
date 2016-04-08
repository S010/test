#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	libusb_context *ctx = NULL;
	libusb_device **devs = NULL;
	ssize_t n;

	if (libusb_init(&ctx) != 0)
		errx(1, "libusb_init");

	n = libusb_get_device_list(ctx, &devs);
	if (n > 0) {
		for (unsigned i = 0; i < n; i++)
			printf("%d:%d\n", libusb_get_bus_number(devs[i]), libusb_get_port_number(devs[i]));
		libusb_free_device_list(devs, n);
	}

	libusb_exit(ctx);

	return 0;
}
