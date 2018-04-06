/* Channel element */
struct msg {
	int type;
	union {
		void *ptr;
		long i;
	} u;
};

/* Channel */
struct msgq {
	struct msg *q;
	int len;
	int max;
	int fd;
};

struct msgq *new_msgq(int max)
{
	struct msgq *msgq;

	msgq = calloc(1, sizeof(*msgq));
	msgq.len = 0;
	msgq.max = max;
	if (max > 0)
		msgq.q = calloc(max, sizeof(*msgq.q));
	return msgq;
}


