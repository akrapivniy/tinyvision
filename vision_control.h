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

struct vcontrol_task {
	int mode;
	int dist;
	int timeout;
	char dir;
	char speed;
	LLIST_STRUCTFIELD
};


int vision_control_init ();
int tasklist_add_tail(LLIST_STRUCT *src);
int vision_control_task_start ();
int is_tasklist_empty();

#endif	/* VISION_CONTROL_H */

