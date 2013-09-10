#include <unistd.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_cond_t start_threads;
pthread_mutex_t mutex;
int counter;

unsigned long
thread_id(void)
{
	return (unsigned long) pthread_self();
}

void *
t(void *arg)
{
	int array_size = * (int *) arg;
	int *arr;
	int i;
	char filename[64];
	FILE *fp;

	arr = malloc(sizeof(int) * array_size);
	snprintf(filename, sizeof filename, "thread_%lx.out", thread_id());
	fp = fopen(filename, "w");
	if (fp == NULL)
		err(1, "fopen");

	if (pthread_mutex_lock(&mutex))
		err(1, "pthread_mutex_lock");
	printf("thread %lx waiting...\n", thread_id());
	if (pthread_cond_wait(&start_threads, &mutex))
		err(1, "pthread_cond_wait");
	if (pthread_mutex_unlock(&mutex))
		err(1, "pthread_mutex_unlock");

	printf("thread %lx working...\n", thread_id());
	for (i = 0; i < array_size; ++i)
		arr[i] = counter++;
	for (i = 0; i < array_size; ++i)
		fprintf(fp, "%i\n", arr[i]);
	fclose(fp);
	free(arr);

	return NULL;
}

int
main(int argc, char **argv)
{
	int i;
	int n_threads;
	int array_size;
	pthread_t *threads;

	if (argc < 3)
		return -1;

	if (pthread_cond_init(&start_threads, NULL))
		err(1, "pthread_cond_init");
	if (pthread_mutex_init(&mutex, NULL))
		err(1, "pthread_mutex_init");

	n_threads = atoi(argv[1]);
	array_size = atoi(argv[2]);

	threads = malloc(sizeof(pthread_t) * n_threads);
	for (i = 0; i < n_threads; ++i)
		if (pthread_create(threads + i, NULL, t, &array_size))
			err(1, "pthread_create");
	sleep(3);
	if (pthread_mutex_lock(&mutex))
		err(1, "pthread_mutex_lock");
	if (pthread_cond_broadcast(&start_threads))
		err(1, "pthread_cond_broadcast");
	if (pthread_mutex_unlock(&mutex))
		err(1, "pthread_mutex_unlock");
	for (i = 0; i < n_threads; ++i)
		if (pthread_join(threads[i], NULL))
			err(1, "pthread_join");
	free(threads);

	if (pthread_cond_destroy(&start_threads))
		err(1, "pthread_cond_destroy");
	if (pthread_mutex_destroy(&mutex))
		err(1, "pthread_mutex_destroy");

	return 0;
}

