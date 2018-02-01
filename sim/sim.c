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
#include <stdbool.h>
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
sync_read(int f, void *buf, size_t size, int timeout_ms)
{
	ssize_t n;
	ssize_t off;

	for (n = off = 0; (size_t)off < size; off += n) {
		struct pollfd pfd = { f, POLLIN, 0 };
		int rc = poll(&pfd, 1, timeout_ms);
		if (rc < 0)
			err(1, "poll");
		if (rc == 0)
			break;
		n = read(f, (uint8_t *)buf + off, size - off);
		if (n < 0)
			return off > 0 ? off : n;
	}
	return off;
}

void
read_decode_atr(int f)
{
	const int max_timeout = 1000; /* ms */

	uint8_t ch;
	uint8_t Y;
	uint8_t K;
	uint8_t tck = 0;
	bool is_tck_present = false;

	#define READ(name, index, ...)								\
		do { 										\
			if (sync_read(f, &ch, 1, max_timeout) != 1)				\
				errx(1, "failed to read " #name "_%d character", index);	\
			tck ^= ch;								\
			if (index != 0)								\
				printf(" " #name "_%d=%02x", index, ch);			\
			else									\
				printf(" " #name "=%02x", ch);					\
			printf(__VA_ARGS__);							\
		} while (0)

	READ(TS, 0, ": %s convention\n", ch == 0x3b ? "direct" : (ch == 0x3f ? "inverse" : "unknown"));
	if (ch != 0x3b)
		errx(1, "unsupported convention");
	tck ^= ch;

	READ(T0, 0, ": Y_1=%#x, K=%u\n", Y = (ch & 0xf0), K = (ch & 0xf));

	int i = 1;
	do {
		if (Y & (1 << 4))
			READ(TA, i, "\n");
		if (Y & (1 << 5))
			READ(TB, i, "\n");
		if (Y & (1 << 6))
			READ(TC, i, "\n");
		if (Y & (1 << 7)) {
			READ(TD, i, ": Y=%#x, T=%d\n", Y = (ch & 0xf0), ch & 0xf);
			if ((ch & 0xf) != 0)
				is_tck_present = true;
		}
	} while (i++, Y & (1 << 7));

	for (i = 1; i <= K+1; i++)
		READ(T, i, "\n");

	if (is_tck_present)
		READ(TCK, 0, "\n");

	if (tck != 0)
		errx(1, "TCK check failed");

	#undef READ
}

void
dump_atr(int f)
{
	uint8_t buf[32];
	int n;
	int i;

	n = sync_read(f, buf, 32, 1000);
	for (i = 0; i < n; ++i)
		printf("%02X ", buf[i]);
	putchar('\n');
}

int
main(int argc, char **argv)
{
	int f;

	(void)argc;
	(void)argv;

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
	read_decode_atr(f);
	//dump_atr(f);

	close(f);

	return EXIT_SUCCESS;
}
