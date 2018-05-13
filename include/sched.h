#ifndef SCHED_H
# define SCHED_H

# include <unistd.h>
# if _WIN32
#  include <windows.h>
# endif

struct scheduler;

typedef void (*taskfunc)(void*, struct scheduler *);

static inline int sched_default_threads()
{
#if _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure);
int sched_spawn(taskfunc f, void *closure, struct scheduler *s);

#endif
