#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define PASTE(x) #x

static int aflag;
static int tflag;

static void
test(const char *msg)
{
    char buf[5 + 22] = "msg: ";
    size_t len;

    len = strlen(buf);
    /* notice that we don't check whether the msg will
     * actually fit in the buf
     */
    strcpy(buf + len, msg);
    printf("%s\n", buf);

    if (aflag)
        abort();
}

static void *
start_test_thread(void *p)
{
    test(p);
    return NULL;
}

int
main(int argc, char **argv, char **envp)
{
    int ch;
    pthread_t thread;
    const char *msg = "ALL YOUR STACK ARE CORRUPTED BY US";

    while ((ch = getopt(argc, argv, "at")) != -1) {
        switch (ch) {
        case 'a':
            ++aflag;
            break;
        case 't':
            ++tflag;
            break;
        case 'h':
        default:
            printf("usage: " PASTE(PROGNAME) " [-a] [-t] [msg]\n"
                   "Deliberately corrupts the stack to cause a crash.\n"
                   "    -a -- call abort(3)\n"
                   "    -t -- run the crashing code in a separate thread\n");
            return 0;
        }
    }
    argc -= optind;
    argv += optind;
    
    if (argc > 0)
        msg = *argv;

    if (tflag) {
        pthread_create(&thread, NULL, start_test_thread, (void *) msg);
        pthread_join(thread, NULL);
    } else {
        test(msg);
    }

    return 0;
}