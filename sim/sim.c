#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

void
write_log(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

const char *
str_ldisc(int ldisc)
{
	switch (ldisc) {
	case TTYDISC: return "TTYDISC";
	case PPPDISC: return "PPPDISC";
	case NMEADISC: return "NMEADISC";
	case MSTSDISC: return "MSTSDISC";
	}
	return "(none)";
}

const char *
str_state(int state)
{
	#define Y(x) if (state & x) { return #x; }
	Y(TIOCM_LE);
	Y(TIOCM_DTR);
	Y(TIOCM_RTS);
	Y(TIOCM_ST);
	Y(TIOCM_SR);
	Y(TIOCM_CTS);
	Y(TIOCM_CAR);
	Y(TIOCM_RNG);
	Y(TIOCM_DSR);
	#undef Y
	return "(unknown)";
}

void
print_line_state(int f)
{
	int error;
	int state;

	error = ioctl(f, TIOCMGET, &state);
	if (error)
		err(1, "ioctl TIOCMGET");
	write_log("Line state: %s (0x%x)\n", str_state(state), (unsigned)state);
}

void
print_line_disc(int f)
{
	int error;
	int ldisc;

	error = ioctl(f, TIOCGETD, &ldisc);
	if (error)
		err(1, "ioctl TIOCGETD");
	write_log("Line discipline: %s (%d)\n", str_ldisc(ldisc), ldisc);
}

void
print_baud_rate(int f)
{
	int error;
	struct termios t;
	speed_t ispeed;
	speed_t ospeed;

	error = tcgetattr(f, &t);
	if (error)
		err(1, "tcgetattr");
	ispeed = cfgetispeed(&t);
	ospeed = cfgetospeed(&t);
	write_log("Line speed: %u input, %u output\n", ispeed, ospeed);
}

void
set_line_state(int f, int new_state)
{
	int error;
	int state;

	error = ioctl(f, TIOCMGET, &state);
	if (error)
		err(1, "ioctl TIOCMGET");

	state |= new_state;
	error = ioctl(f, TIOCMSET, &state);
	if (error)
		err(1, "ioctl TIOCMSET");
	print_line_state(f);
}

void
clear_line_state(int f, int bits)
{
	int error;
	int state;

	error = ioctl(f, TIOCMGET, &state);
	if (error)
		err(1, "ioctl TIOCMGET");

	state &= ~bits;
	error = ioctl(f, TIOCMSET, &state);
	if (error)
		err(1, "ioctl TIOCMSET");
	print_line_state(f);
}

void
set_baud_rate(int f, speed_t rate)
{
	int error;
	struct termios t;
	speed_t ispeed;
	speed_t ospeed;

	error = ioctl(f, TIOCGETA, &t);
	if (error)
		err(1, "ioctl TIOCGETA");
	cfsetspeed(&t, rate);
	print_baud_rate(f);
}

void
set_raw_io(int f)
{
	int error;
	struct termios t;

	error = tcgetattr(f, &t);
	if (error)
		err(1, "tcgetattr");
	cfmakeraw(&t);
	error = tcsetattr(f, TCSAFLUSH, &t);
	if (error)
		err(1, "tcsetattr");
	write_log("Set raw I/O mode.\n");
}

void
set_nonblock(int f)
{
	int error;

	error = fcntl(f, F_SETFL, O_NONBLOCK);
	if (error)
		err(1, "fcntl F_SETFL O_NONBLOCK");
	write_log("Set non-blocking I/O mode\n");
}

void
print_byte(uint8_t byte)
{
	static const char hex[] = "0123456789abcdef";
	putchar(hex[byte >> 4]);
	putchar(hex[byte & 0xf]);
}

void
flush_queues(int f)
{
	int error;
	const int what = FREAD | FWRITE;

	error = ioctl(f, TIOCFLUSH, &what);
	if (error)
		err(1, "ioctl TIOCFLUSH");
}

void
sleep_for_ms(unsigned ms)
{
	struct timespec t;
	struct timespec r;
	int rc;
	
	t.tv_sec = ms / 1000;
	t.tv_nsec = (ms % 1000) * 1000 * 1000;
	do {
		rc = nanosleep(&t, &r);
		t = r;
	} while (rc == -1 && errno == EINTR);
}

int
main(int argc, char **argv)
{
	int f;
	int error;
	int state;

	f = open("/dev/cuaU0", O_RDWR);
	if (f == -1)
		err(1, "open");
	write_log("Opened device node.\n");

	print_line_disc(f);
	print_line_state(f);
	print_baud_rate(f);

	write_log("Configuring serial line...\n");
	set_nonblock(f);
	set_raw_io(f);
	set_baud_rate(f, B9600);

	write_log("Resetting ICC...\n");
	clear_line_state(f, TIOCM_DTR);
	sleep_for_ms(200);
	flush_queues(f);
	set_line_state(f, TIOCM_DTR);

	write_log("Reading ATR...\n");
	for (;;) {
		struct pollfd pfd = { f, POLLIN };
		int rc = poll(&pfd, 1, INFTIM);
		if (rc < 0)
			err(1, "poll");
		if (rc == 0)
			continue;
		uint8_t byte;
		while (read(f, &byte, 1) > 0)
			print_byte(byte);
		putchar('\n');
	}

	close(f);

	return EXIT_SUCCESS;
}
