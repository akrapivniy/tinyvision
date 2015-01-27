#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "vision_control.h"



/*
O=O=0+--NULL
+----+
 */

LLIST_STRUCT *llist = NULL;
LLIST_STRUCT *llist_free = NULL;
pthread_mutex_t llist_lock;
int vision_task_timeout = 0;

static inline LLIST_STRUCT *llist_get_free()
{
	LLIST_STRUCT *free = NULL;

	if (llist_free != NULL) {
		free = llist_free;
		if (llist_free->next == NULL) {
			llist_free = NULL;
		} else {
			llist_free = (LLIST_STRUCT *) free->next;
			llist_free->prev = free->prev;
		}

	} else {
		free = (LLIST_STRUCT *) malloc(sizeof(LLIST_STRUCT));
	}

	return free;
}

static inline void llist_add_free(LLIST_STRUCT *item)
{
	LLIST_STRUCT *tail = NULL;

	if (llist_free == NULL) {
		llist_free = item;
		item->prev = item;
		item->next = NULL;
		return;
	}
	item->next = NULL;
	item->prev = llist_free->prev;
	tail = llist_free->prev;
	tail->next = item;
	llist_free->prev = item;
}

static inline void llist_add_free_all(LLIST_STRUCT *item)
{
	LLIST_STRUCT *tail = NULL;
	LLIST_STRUCT *tail_src = NULL;

	if (llist_free == NULL) {
		llist_free = item;
		return;
	}

	tail = llist_free->prev;
	tail_src = item->prev;

	item->prev = tail;

	tail->next = item;
	llist_free->prev = tail_src;
}

static inline void llist_release_head()
{
	LLIST_STRUCT *free = NULL;

	if (llist == NULL) return;

	free = llist;
	llist = llist -> next;

	if (llist == NULL) return;

	llist->prev = free->prev;

	llist_add_free(free);
	return;
}

static inline void llist_add_head(LLIST_STRUCT *item)
{
	LLIST_STRUCT *tail = NULL;

	if (llist == NULL) {
		llist = item;
		item->prev = item;
		item->next = NULL;
		return;
	}
	item->next = llist;
	item->prev = llist->prev;
	llist = item;
}

static inline void llist_add_tail(LLIST_STRUCT *item)
{
	LLIST_STRUCT *tail = NULL;

	if (llist == NULL) {
		llist = item;
		item->prev = item;
		item->next = NULL;
		return;
	}
	item->next = NULL;
	item->prev = llist->prev;
	tail = llist->prev;
	tail->next = item;
	llist->prev = item;
}

static inline void llist_release_all()
{
	LLIST_STRUCT *free = NULL;

	if (llist == NULL) return;
	free = llist;
	llist = NULL;

	llist_add_free_all(free);
	return;
}

int tasklist_add_tail(LLIST_STRUCT *src)
{
	LLIST_STRUCT *dst;
	int err = 0;

	pthread_mutex_lock(&llist_lock);

	dst = llist_get_free();
	if (dst == NULL) {
		err = -1;
		goto exit;
	}

	memcpy(dst, src, sizeof(LLIST_STRUCT));
	llist_add_tail(dst);

exit:
	pthread_mutex_unlock(&llist_lock);
	vision_control_task_start ();
	return err;
}

int tasklist_add_head(LLIST_STRUCT *src)
{

	LLIST_STRUCT *dst;
	int err = 0;

	pthread_mutex_lock(&llist_lock);

	dst = llist_get_free();
	if (dst == NULL) {
		err = -1;
		goto exit;
	}

	memcpy(dst, src, sizeof(LLIST_STRUCT));
	llist_add_head(dst);

exit:
	pthread_mutex_unlock(&llist_lock);
	vision_control_task_start ();
	return err;
}

int tasklist_get_next(LLIST_STRUCT *dst)
{
	LLIST_STRUCT *src;
	int err = 0;

	pthread_mutex_lock(&llist_lock);

	src = llist;
	if (src == NULL) {
		err = -1;
		goto exit;
	}

	memcpy(dst, src, sizeof(LLIST_STRUCT));
	llist_release_head();


exit:
	pthread_mutex_unlock(&llist_lock);
	return err;
}

int tasklist_clean()
{
	int err = 0;

	pthread_mutex_lock(&llist_lock);
	llist_release_all();

exit:
	pthread_mutex_unlock(&llist_lock);
	return err;
}

int is_tasklist_empty()
{
	return ((llist == NULL)? 1 : 0);
}

int tasklist_init()
{
	llist = NULL;
	llist_free = NULL;

	if (pthread_mutex_init(&llist_lock, NULL) != 0)
		return -1;

}

int tasklist_release_free()
{
	int err = 0;
	LLIST_STRUCT *l;

	pthread_mutex_lock(&llist_lock);

	while (llist_free) {
		l = llist_free;
		llist_free = (LLIST_STRUCT *)l->next;
		free(l);
	};

exit:
	pthread_mutex_unlock(&llist_lock);
	return err;
}

int tasklist_close()
{
	pthread_mutex_destroy(&llist_lock);
}

int vision_control_task_next()
{
	LLIST_STRUCT task;

	if (tasklist_get_next(&task) < 0)
		return 0;

	motor_set_control(task.dir, task.speed, task.dist, &vision_control_task_next);
	vision_task_timeout = task.timeout+1;
	printf ("[get timesout = %d]",task.timeout);fflush(stdout);
	return 0;
}

int vision_control_task_start()
{
	if (vision_task_timeout == 0)
		vision_control_task_next();
}

int vision_control_task_stop()
{
	vision_task_timeout = 0;
	motor_set_control('S', 0, 0, NULL);
	tasklist_clean ();
}

static void* vision_control_timeout(void *p)
{
	int sig;
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);

	while (!sigwait(&sigset, &sig)) {
		printf ("[%d]",vision_task_timeout);fflush(stdout);
		if (vision_task_timeout) {
			vision_task_timeout--;
		} else continue;
		if (vision_task_timeout == 1) {
			vision_control_task_next();
		} 
	}
}

int vision_control_init()
{
	pthread_t thread;
	timer_t timer;
	sigset_t sigset;
	struct sigevent se = {0};
	struct itimerspec ts = {0};

	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_signo = SIGUSR1;

	ts.it_value.tv_sec = 1;
	ts.it_interval.tv_sec = 1;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	pthread_create(&thread, NULL, vision_control_timeout, NULL);

	timer_create(CLOCK_REALTIME, &se, &timer);

	timer_settime(timer, 0, &ts, NULL);

	tasklist_init();
}