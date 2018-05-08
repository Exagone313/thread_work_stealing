#include <errno.h>
#include <stdlib.h>
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
} t_state;

/* Must not return 0 */
static void *strategy_storage_init(void *state)
{
	// FIXME is it needed to save in each thread its id, so we can know here
	// if all threads are sleeping?
	// if so, need to lock mutex in state etc...
	(void)state;
	return (void *)1;
}

static t_task_callback *strategy_get_task(t_scheduler *sched, void *storage)
{
	t_state *state;

	state = (t_state *)sched->state;
	(void)storage;
	/* TODO
	 * lock mutex in state
	 * get task in state lifo
	 * unlock */
	// TODO set quit = 1 when needed (lock?)
	return NULL;
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
	state.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; // TODO destroy mutex
	pthread_mutex_lock(&(state.mutex));
	state.task_list[0].f = f;
	state.task_list[0].closure = closure;
	task_init(&sched, nthreads,
			strategy_storage_init, strategy_get_task, &state);
	if (sched.quit)
		r = -1;
	else
		r = 0;
	pthread_mutex_unlock(&(state.mutex));
	// TODO must block until it ends (if r == 0)
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
