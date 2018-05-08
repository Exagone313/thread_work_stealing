#include <stdlib.h>
#include "task.h"

/* FIXME what if we'd wait for a fixed time
and send signals to sleeping threads?
=> pthread_kill, pthread_self..?
OR cond var?
*/

/*
static int task_sleep_time(t_scheduler *sched, int reset)
{
	int r;

	pthread_spin_lock(&(sched->sleep_lock));
	if (reset)
	{
		sched->sleep_time = 1;
		r = 0;
	}
	else
		r = sched->sleep_time++;
	pthread_spin_unlock(&(sched->sleep_lock));
	return r;
}

static void task_sleep(t_scheduler *sched)
{
	(void)sched;
}
*/

static void *task_thread(void *arg)
{
	t_scheduler *sched;
	void *storage;
	t_task_callback cb;
	int r;

	sched = (t_scheduler *)arg;
	if(!(storage = sched->storage_init(sched)))
		pthread_exit(NULL);
	while (1)
	{
		if ((r = sched->get_task(&cb, sched->state, storage)))
		{
			if (r == 1) // quit
				break;
			// TODO wait
		}
		else
			cb.f(cb.closure, sched);
	}
	pthread_exit(NULL);
}

void task_init(t_scheduler *sched, int nthreads,
		t_strategy_storage_init storage_init,
		t_strategy_get_task get_task,
		void *state)
{
	int i;
	int r;

	sched->quit = 0;
	if (!(sched->thread_list = malloc(sizeof(*sched->thread_list) * nthreads)))
	{
		sched->quit = 1;
		return;
	}
	/*if (pthread_spin_init(&(sched->sleep_lock), PTHREAD_PROCESS_PRIVATE))
	{
		free(sched->thread_list);
		sched->quit = 1;
		return;
	}*/
	for (i = 0; i < nthreads; i++)
	{
		if (pthread_create(&(sched->thread_list[i]), NULL,
					task_thread, NULL))
		{
			sched->quit = 1;
			break;
		}
	}
	sched->thread_count = i;
}

void task_wait(t_scheduler *sched)
{
	int i;

	for (i = 0; i < sched->thread_count; i++)
	{
		pthread_join(sched->thread_list[i], NULL); // TODO get return value (error)
	}
	free(sched->thread_list);
	//pthread_spin_destroy(&(sched->sleep_lock));
}
