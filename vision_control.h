/* 
 * File:   vision_control.h
 * Author: akrapivniy
 *
 * Created on 26 Январь 2015 г., 20:54
 */

#ifndef VISION_CONTROL_H
#define	VISION_CONTROL_H

#define LLIST_STRUCT struct vcontrol_task
#define LLIST_STRUCTFIELD void *next, *prev;

#define VTASK_QUEUE	0x0
#define VTASK_START	0x1
#define VTASK_COMPLITE	0x2 
#define VTASK_TIMEOUT	0x3
#define VTASK_ABORT	0x4

struct vcontrol_task {
	int mode;
	int dist;
	int timeout;
	char dir;
	char speed;
	int *state;
	LLIST_STRUCTFIELD
};


int vision_control_init();
int tasklist_add_tail(LLIST_STRUCT *src);
int vision_control_task_start();
int vision_control_task_stop();
int vision_control_task_pause();
int vision_control_task_restore();
int vision_control_task_dangerous();
int is_tasklist_empty();

int tasklist_fixed_current_task(int key, char dir, int speed);

#endif	/* VISION_CONTROL_H */

