#include "sched.h"

struct scheduler
{
	char ignore;
};

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
	(void)nthreads;
	(void)qlen;
	f(closure, NULL);
	return 0;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *s)
{
	(void)s;
	f(closure, NULL);
	return 0;
}
