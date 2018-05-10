#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

typedef struct s_state
{
	pthread_mutex_t mutex;
	t_task_callback *task_list;
	int task_count;
	int task_size;
} t_state;

static void *strategy_storage_init(t_scheduler *sched)
{
	(void)sched;
	return (void *)1;
}

static int strategy_get_task(t_task_callback *cb, t_scheduler *sched,
		void *storage_ptr)
{
	t_state *state;
	int r;
	int i;

	(void)storage_ptr;
	state = (t_state *)sched->state;
	pthread_mutex_lock(&state->mutex);
	if (sched->quit)
		r = 1;
	else if (state->task_count == 0)
	{
		for (i = 0; i < sched->thread_count; i++)
		{
			if (sched->thread_list[i].thread == pthread_self())
				break;
		}
		// could assert i < sched->thread_count
		sched->thread_list[i].sleeping = 1;
		for (i = 0; i < sched->thread_count; i++) // XXX
		{
			if (!sched->thread_list[i].sleeping)
				break;
		}
		if (i == sched->thread_count) // all threads sleeping
		{
			sched->quit = 1;
			r = 1;
		}
		else // TODO cond wait loop
			r = -1;
	}
	else
	{
		state->task_count--;
		memcpy(cb, &(state->task_list[state->task_count]), sizeof(*cb));
		for (i = 0; i < sched->thread_count; i++)
		{
			if (sched->thread_list[i].thread == pthread_self())
				break;
		}
		// could assert i < sched->thread_count
		sched->thread_list[i].sleeping = 0;
		r = 0;
	}
	pthread_mutex_unlock(&state->mutex);
	return r;
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
	t_state state;
	t_scheduler sched;
	int r;

	if (nthreads == -1)
		nthreads = sched_default_threads();
	if (qlen <= 0 || nthreads <= 0)
		return -1;
	if (!(state.task_list = malloc(sizeof(*state.task_list) * qlen)))
		return -1;
	state.task_count = 1;
	state.task_size = qlen;
	state.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&state.mutex);
	state.task_list[0].f = f;
	state.task_list[0].closure = closure;
	task_init(&sched, nthreads,
			strategy_storage_init, strategy_get_task, &state);
	if (sched.quit)
		r = -1;
	else
		r = 0;
	pthread_mutex_unlock(&state.mutex);
	task_wait(&sched);
	pthread_mutex_destroy(&state.mutex);
	free(state.task_list);
	return r;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *sched)
{
	t_state *state;
	int r;

	state = (t_state *)sched->state;
	pthread_mutex_lock(&state->mutex);
	if (state->task_count == state->task_size)
	{
		errno = EAGAIN;
		r = -1;
	}
	else
	{
		state->task_list[state->task_count].f = f;
		state->task_list[state->task_count].closure = closure;
		state->task_count++;
		r = 0; // TODO cond signal/broadcast
	}
	pthread_mutex_unlock(&state->mutex);
	return r;
}
