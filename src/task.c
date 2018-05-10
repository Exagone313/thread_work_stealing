#include <stdlib.h>
#include <unistd.h>
#include "task.h"

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
		if ((r = sched->get_task(&cb, sched, storage)))
		{
			if (r == 1) // quit
				break;
			// TODO better wait
			sleep(1);
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

	if (!(sched->thread_list = calloc(1,
					sizeof(*sched->thread_list) * nthreads)))
	{
		sched->quit = 1;
		return;
	}
	sched->quit = 0;
	sched->storage_init = storage_init;
	sched->get_task = get_task;
	sched->state = state;
	for (i = 0; i < nthreads; i++)
	{
		if (pthread_create(&sched->thread_list[i].thread, NULL,
					task_thread, sched))
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
		pthread_join(sched->thread_list[i].thread, NULL); // TODO get return value (error)
	free(sched->thread_list);
}
