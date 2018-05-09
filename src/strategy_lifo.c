#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

/*
*/
//static int task_add(t_task_box *box, taskfunc f, void *closure);

typedef struct s_state
{
	pthread_mutex_t mutex;
	t_task_callback *task_list;
	int task_count;
	int task_size;
	int thread_id;
	int *thread_sleeping;
} t_state;

typedef struct s_storage
{
	int id;
} t_storage;

/* Must not return 0 */
static void *strategy_storage_init(t_scheduler *sched)
{
	t_state *state;
	t_storage *r;

	state = (t_state *)sched->state;
	pthread_mutex_lock(&(state->mutex));
	if (sched->quit)
		return NULL;
	if (!(r = malloc(sizeof(*r))))
	{
		sched->quit = 1;
		return NULL;
	}
	r->id = state->thread_id++;
	pthread_mutex_unlock(&(state->mutex));
	return r;
}

static int strategy_get_task(t_task_callback *cb, t_scheduler *sched,
		void *storage_ptr)
{
	t_state *state;
	int r;
	int i;
	t_storage *storage;

	state = (t_state *)sched->state;
	storage = (t_storage *)storage_ptr;
	pthread_mutex_lock(&(state->mutex));
	if (sched->quit)
		r = 1;
	else if (state->task_count == 0)
	{
		state->thread_sleeping[storage->id] = 1;
		for (i = 0; i < sched->thread_count; i++)
		{
			if (!state->thread_sleeping[i])
				break;
		}
		if (i == sched->thread_count) // all threads sleeping
		{
			sched->quit = 1;
			r = 1;
		}
		else
			r = -1;
	}
	else
	{
		state->task_count--;
		memcpy(cb, &(state->task_list[state->task_count]), sizeof(*cb));
		state->thread_sleeping[storage->id] = 0;
		r = 0;
	}
	pthread_mutex_unlock(&(state->mutex));
	return r;
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
	t_state state;
	t_scheduler sched;
	int r;

	if (qlen <= 0 || nthreads <= 0)
		return -1;
	if (!(state.task_list = malloc(sizeof(*state.task_list) * qlen)))
		return -1;
	state.task_count = 1;
	state.task_size = qlen;
	state.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&(state.mutex));
	state.task_list[0].f = f;
	state.task_list[0].closure = closure;
	state.thread_id = 0;
	task_init(&sched, nthreads,
			strategy_storage_init, strategy_get_task, &state);
	if (sched.quit || !(state.thread_sleeping = calloc(1,
					sizeof(*state.thread_sleeping))))
		r = -1;
	else
		r = 0;
	pthread_mutex_unlock(&(state.mutex));
	task_wait(&sched);
	pthread_mutex_destroy(&(state.mutex));
	free(state.task_list);
	return r;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *s)
{
	t_state *state;

	state = (t_state *)s->state;
	pthread_mutex_lock(&(state->mutex));
	if (state->task_count == state->task_size)
	{
		errno = EAGAIN;
		return -1;
	}
	state->task_list[state->task_count].f = f;
	state->task_list[state->task_count].closure = closure;
	state->task_count++;
	pthread_mutex_unlock(&(state->mutex));
	return 0;
}
