#ifndef TASK_H
# define TASK_H

# include <pthread.h>
# include "sched.h"

typedef struct s_task_callback
{
	taskfunc f;
	void *closure;
} t_task_callback;

typedef struct scheduler t_scheduler;

/* Initialize storage for current thread
 * return NULL if memory allocation failed
 */
typedef void *(*t_strategy_storage_init)(t_scheduler *sched);

/* Get a task to execute
 * return NULL if no task is available
 * Note: each strategy_storage is in a variable in thread
 */
typedef int (*t_strategy_get_task)(t_task_callback *cb,
		t_scheduler *sched, void *storage);

struct scheduler
{
	pthread_t *thread_list;
	int thread_count;
	t_strategy_storage_init storage_init;
	t_strategy_get_task get_task;
	void *state;
	int quit;
};

void task_init(t_scheduler *sched, int nthreads,
		t_strategy_storage_init storage_init,
		t_strategy_get_task get_task,
		void *state);

void task_wait(t_scheduler *sched);

#endif
