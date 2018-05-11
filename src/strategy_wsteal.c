#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

/*
XXX

state.cond for signalling ONE sleeping thread when a task is added
* problem, would be better to retrieve what was the dequeue that got a new task!
=> state.dequeue_signaled protected by state.mutex
mutex per dequeue for locking it (needed)
* think about trylock? shouldn't be needed, think again
like for lifo, state.cond broadcast for quiting

algo:
lock dequeue.mutex
got a task? => yes, break through
unlock dequeue.mutex // XXX ???
work steeling on "(last_stolen_dequeue + 0..n) % n" until last one without current
* need to check we won't dead lock
* could have one loop with trylock then one with lock? time loss?
=> if it succeeds, save the id in dequeue.last_stolen_dequeue
lock state.mutex
set current thread as sleeping
wait state.cond (state.mutex)
read dequeue_signaled to know what dequeue got a task // XXX not work stealing?
/!\ XXX how to check if all threads are sleeping?

work steeling:
lock dequeue mutex
got a task? => yes, break through
unlock dequeue mutex

when adding a task:
signal state.cond

when quitting:
broadcast state.cond

FIXME can multiple work stealing happen at the same time?
=> if not, lock state.mutex
FIXME need to lock dequeue.mutex anyway

during work stealing, we lock each other dequeue, could check
thread_unit.sleeping?
how? could save a pointer to it in storage_init, if NULL, not initialized (not sleeping)
FIXME we don't lock the dequeue in storage_init

to add a task, we necessarily need to lock state.mutex, because sleeping threads
uses state.cond to wait for a task

XXX
*/

typedef struct s_dequeue
{
	t_task_callback *task_list;
	int task_start;
	int task_count;
	//pthread_cond_t cond; // TODO mutex/cond thinking + write algo
	pthread_mutex_t mutex; // XXX
	int last_stolen_dequeue; // private, no need for locking
	t_thread_unit *unit;
} t_dequeue;

typedef struct s_state
{
	pthread_mutex_t mutex;
	pthread_cond_t cond; // XXX only used with state.mutex ok?
	t_task_callback *task_list_memory; // allocated for all dequeues at once
	t_dequeue *dequeue; // dequeue list
	int task_size; // static
	int thread_id; // incremented for storage_init
	t_task_callback initial_callback; // copied in first get_task call
	int dequeue_signaled; // XXX rename?
} t_state;

/* Note: I chose to switch top and bottom dequeue operations
 * of the work stealing algorithm, because adding task is easier that way:
 * no need to subtract, and I prefer not to remember how languages handle
 * negative number modulo (see C vs Python).
 */

/* top */
static int get_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	printf("get_task task_count=%d\n", dequeue->task_count);fflush(stdout);
	if (dequeue->task_count == 0) // empty
		return -1;
	dequeue->task_count--;
	memcpy(cb, &dequeue->task_list[(dequeue->task_start + dequeue->task_count)
			% task_size], sizeof(*cb));
	//printf(">> get_task copied f=%p\n", cb->f);fflush(stdout);
	return 0;
}

/* bottom */
static int steal_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	if (dequeue->task_count == 0) // empty
		return -1;
	memcpy(cb, &dequeue->task_list[dequeue->task_start], sizeof(*cb));
	dequeue->task_start = (dequeue->task_start + 1) % task_size;
	return 0;
}

/* top */
static int add_task(t_task_callback *cb, t_dequeue *dequeue, int task_size)
{
	if (dequeue->task_count == task_size) // full
		return -1;
	memcpy(&dequeue->task_list[(dequeue->task_start + dequeue->task_count)
			% task_size], cb, sizeof(*cb));
	printf("<< add_task dq=%p s=%d id=%d l_before=%d\n", dequeue, dequeue->task_start,
			(dequeue->task_start + dequeue->task_count) % task_size, dequeue->task_count);fflush(stdout);
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
	//int max;
	int sleeping;

	dequeue = (t_dequeue *)storage_ptr;
	state = (t_state *)sched->state;
	pthread_mutex_lock(&dequeue->mutex);
	/*else if (sched->quit) // FIXME need locking state.mutex?
		r = 1;*/
	/*sched->quit = 1;
	r = 1;*/
	// TODO before/after? work steeling, we lock state.mutex, need to check sched->quit
	r = get_task(cb, dequeue, state->task_size);
	pthread_mutex_unlock(&dequeue->mutex);
	if (r)
	{
		pthread_mutex_lock(&state->mutex);
		printf("lock state mutex %ld\n", dequeue->unit->thread);fflush(stdout);
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
				//max = (s + sched->thread_count) % sched->thread_count;
				sleeping = 1;
				//printf("for max=%d\n", max);fflush(stdout);
				/*for (i = s % sched->thread_count; i != max;
						i = (i + 1) % sched->thread_count)*/
				for (i = 0; i < sched->thread_count; i++)
				{
					j = (i + s) % sched->thread_count;
					printf("for i=%d j=%d\n", i, j);fflush(stdout);
					if (&state->dequeue[j] != dequeue) // not current thread
					{
						printf("test dq %d\n", j);fflush(stdout);
						pthread_mutex_lock(&state->dequeue[j].mutex);
						printf("lockED dq %d: start=%d, count=%d\n", j,
								state->dequeue[j].task_start,
								state->dequeue[j].task_count);fflush(stdout);
						if (steal_task(cb, &state->dequeue[j],
									state->task_size) == 0)
						{
							printf("STOLEN TASK in dq %d, f=%p\n", j, cb->f);fflush(stdout);
							pthread_mutex_unlock(&state->dequeue[j].mutex);
							printf("unlockED stolen dq %d\n", j);fflush(stdout);
							break;
						}
						printf("no stolen task in dq %d\n", j);fflush(stdout);
						if (state->dequeue[j].unit
								&& state->dequeue[j].unit->sleeping)
							sleeping++;
						printf("unlock dq %d\n", j);fflush(stdout);
						pthread_mutex_unlock(&state->dequeue[j].mutex);
						printf("unlockED dq %d\n", j);fflush(stdout);
					}
				}
				printf("for end i=%d (j=%d)\n", i, j);fflush(stdout);
				//if (i != max) // got a task
				if (i < sched->thread_count) // got a task
				{
					printf("STOLEN TASK (after) in dq %d, f=%p\n", j, cb->f);fflush(stdout);
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
				// TODO set + check sleeping???
				dequeue->unit->sleeping = 1;
				// FIXME not supposed to lock on dequeue mutex + cond to be signaled?
				printf("state cond wait %ld\n", dequeue->unit->thread);fflush(stdout);
				pthread_cond_wait(&state->cond, &state->mutex);
				printf("state cond waitED %ld\n", dequeue->unit->thread);fflush(stdout);
				s = state->dequeue_signaled;
			}
		}
		pthread_mutex_unlock(&state->mutex);
		//pthread_mutex_lock(&dequeue->mutex);
	}
	else
	{
		printf("got task f=%p\n", cb->f);fflush(stdout);
	}
	//pthread_mutex_unlock(&dequeue->mutex);
	printf(">> r=%d f=%p\n", r, cb->f);fflush(stdout);
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
		//state.dequeue[i].cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		state.dequeue[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; // XXX
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
	{
		//pthread_cond_destroy(&state.dequeue[i].cond);
		pthread_mutex_destroy(&state.dequeue[i].mutex); // XXX
	}
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
	add_task(&cb, dequeue, state->task_size);
	pthread_mutex_unlock(&dequeue->mutex);
	pthread_mutex_lock(&state->mutex);
	state->dequeue_signaled = i;
	printf("dq sign=%d\n", i);fflush(stdout);
	pthread_cond_signal(&state->cond);
	pthread_mutex_unlock(&state->mutex);
	return 0;
}
