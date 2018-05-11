#include <stdlib.h>
#include <unistd.h>
#include "task.h"

static void *task_thread(void *arg)
{
	t_thread_unit *unit;
	t_task_callback cb;
	int r;

	unit = (t_thread_unit *)arg;
	printf("start %ld\n", unit->thread);fflush(stdout);
	if(!(unit->storage = unit->sched->storage_init(unit)))
		pthread_exit((void *)1);
	while (1)
	{
		if ((r = unit->sched->get_task(&cb, unit->sched, unit->storage)))
		{
			if (r == 1) // quit
				break;
		}
		else
		{
			if (!cb.f)
			{
				printf("function NULL!\n");fflush(stdout);abort();
			}
			cb.f(cb.closure, unit->sched);
		}
	}
	printf("exit %ld\n", unit->thread);fflush(stdout);
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
		sched->thread_list[i].sched = sched;
		if (pthread_create(&sched->thread_list[i].thread, NULL,
					task_thread, &sched->thread_list[i]))
		{
			sched->quit = 1;
			break;
		}
	}
	sched->thread_count = i;
}

int task_wait(t_scheduler *sched)
{
	int i;
	int r;
	void *v;

	r = 0;
	for (i = 0; i < sched->thread_count; i++)
	{
		pthread_join(sched->thread_list[i].thread, &v);
		if (v)
			r = -1;
	}
	free(sched->thread_list);
	return r;
}
