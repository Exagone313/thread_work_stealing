#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

typedef struct s_dequeue
{
	t_task_callback *task_list;
	int task_start;
	int task_count;
	pthread_mutex_t mutex;
	int last_stolen_dequeue; // private, no need for locking
	t_thread_unit *unit;
} t_dequeue;

typedef struct s_state
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	t_task_callback *task_list_memory; // allocated for all dequeues at once
	t_dequeue *dequeue; // dequeue list
	int task_size; // static
	int thread_id; // incremented for storage_init
	t_task_callback initial_callback; // copied in first get_task call
	int dequeue_signaled;
} t_state;

/* Note: I chose to switch top and bottom dequeue operations
 * of the work stealing algorithm, because adding task is easier that way:
 * no need to subtract, and I prefer not to remember how languages handle
 * negative number modulo (see C vs Python).
 */

/* top */
static int get_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	if (dequeue->task_count == 0) // empty
		return -1;
	dequeue->task_count--;
	memcpy(cb, &dequeue->task_list[(dequeue->task_start + dequeue->task_count)
			% task_size], sizeof(*cb));
	return 0;
}

/* bottom */
static int steal_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	if (dequeue->task_count == 0) // empty
		return -1;
	memcpy(cb, &dequeue->task_list[dequeue->task_start], sizeof(*cb));
	dequeue->task_start = (dequeue->task_start + 1) % task_size;
	dequeue->task_count--;
	return 0;
}

/* top */
static int add_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	if (dequeue->task_count == task_size) // full
		return -1;
	memcpy(&dequeue->task_list[(dequeue->task_start + dequeue->task_count)
			% task_size], cb, sizeof(*cb));
	dequeue->task_count++;
	return 0;
}

static void *strategy_storage_init(t_thread_unit *unit)
{
	t_state *state;
	t_dequeue *r;

	state = (t_state *)unit->sched->state;
	pthread_mutex_lock(&state->mutex);
	r = &state->dequeue[state->thread_id++];
	r->unit = unit;
	if (state->initial_callback.f)
	{
		pthread_mutex_lock(&r->mutex);
		add_task(&state->initial_callback, r, state->task_size);
		pthread_mutex_unlock(&r->mutex);
		state->initial_callback.f = NULL;
	}
	pthread_mutex_unlock(&state->mutex);
	return r;
}

static int strategy_get_task(t_task_callback *cb, t_scheduler *sched,
		void *storage_ptr)
{
	t_state *state;
	t_dequeue *dequeue;
	int r;
	int i;
	int s;
	int j;
	int sleeping;

	dequeue = (t_dequeue *)storage_ptr;
	state = (t_state *)sched->state;
	pthread_mutex_lock(&dequeue->mutex);
	r = get_task(cb, dequeue, state->task_size);
	pthread_mutex_unlock(&dequeue->mutex);
	if (r)
	{
		pthread_mutex_lock(&state->mutex);
		s = dequeue->last_stolen_dequeue >= 0 ? dequeue->last_stolen_dequeue
			: (int)((dequeue - state->dequeue) / sizeof(*dequeue)) + 1;
		while (1)
		{
			if (sched->quit)
			{
				r = 1;
				break;
			}
			else
			{
				sleeping = 1;
				for (i = 0; i < sched->thread_count; i++)
				{
					j = (i + s) % sched->thread_count;
					if (&state->dequeue[j] != dequeue) // not current thread
					{
						pthread_mutex_lock(&state->dequeue[j].mutex);
						if (steal_task(cb, &state->dequeue[j],
									state->task_size) == 0)
						{
							pthread_mutex_unlock(&state->dequeue[j].mutex);
							break;
						}
						if (state->dequeue[j].unit
								&& state->dequeue[j].unit->sleeping)
							sleeping++;
						pthread_mutex_unlock(&state->dequeue[j].mutex);
					}
				}
				if (i < sched->thread_count) // got a task
				{
					r = 0;
					dequeue->last_stolen_dequeue = j; // XXX i
					break;
				}
				if (sleeping == sched->thread_count)
				{
					sched->quit = 1;
					r = 1;
					pthread_cond_broadcast(&state->cond);
					break;
				}
				dequeue->unit->sleeping = 1;
				pthread_cond_wait(&state->cond, &state->mutex);
				s = state->dequeue_signaled;
			}
		}
		pthread_mutex_unlock(&state->mutex);
	}
	return r;
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
	t_state state;
	t_scheduler sched;
	int r;
	int i;

	if (nthreads == -1)
		nthreads = sched_default_threads();
	if (qlen <= 0 || nthreads <= 0)
		return -1;
	if (!(state.task_list_memory = malloc(sizeof(*state.task_list_memory)
					* qlen * nthreads)))
		return -1;
	if (!(state.dequeue = calloc(1, sizeof(*state.dequeue) * nthreads)))
	{
		free(state.task_list_memory);
		return -1;
	}
	state.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	state.cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	state.task_size = qlen;
	state.thread_id = 0;
	state.initial_callback.f = f;
	state.initial_callback.closure = closure;
	state.dequeue_signaled = 0;
	for (i = 0; i < nthreads; i++)
	{
		state.dequeue[i].task_list = &state.task_list_memory[i * qlen];
		state.dequeue[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		state.dequeue[i].last_stolen_dequeue = -1;
	}
	pthread_mutex_lock(&state.mutex);
	task_init(&sched, nthreads,
			strategy_storage_init, strategy_get_task, &state);
	if (sched.quit)
		r = -1;
	else
		r = 0;
	pthread_mutex_unlock(&state.mutex);
	if (task_wait(&sched))
		r = -1;
	for (i = 0; i < nthreads; i++)
		pthread_mutex_destroy(&state.dequeue[i].mutex);
	pthread_cond_destroy(&state.cond);
	pthread_mutex_destroy(&state.mutex);
	free(state.dequeue);
	free(state.task_list_memory);
	return r;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *s)
{
	t_state *state;
	t_dequeue *dequeue;
	t_task_callback cb;
	int i;
	int r;

	state = (t_state *)s->state;
	for (i = 0; i < s->thread_count; i++)
	{
		if (pthread_equal(s->thread_list[i].thread, pthread_self()))
			break;
	}
	// could assert i < s->thread_count
	dequeue = (t_dequeue *)s->thread_list[i].storage;
	cb.f = f;
	cb.closure = closure;
	pthread_mutex_lock(&dequeue->mutex);
	r = add_task(&cb, dequeue, state->task_size);
	pthread_mutex_unlock(&dequeue->mutex);
	if (r == 0)
	{
		pthread_mutex_lock(&state->mutex);
		state->dequeue_signaled = i;
		pthread_cond_signal(&state->cond);
		pthread_mutex_unlock(&state->mutex);
	}
	else
		errno = EAGAIN;
	return 0;
}
