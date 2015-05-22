#include <stdio.h>
#include <string.h>
#include "vision_core.h"
#include "vision_sframe.h"
#include "vision_floor.h"
#include "vision_control.h"
#include "vision_sframe.h"



#define VISION_HIDE_RUN  0
#define VISION_HIDE_BACK 32
#define VISION_HIDE_BACK2 34
#define VISION_HIDE_END 0xff

struct vision_hide_state {
	int phase;
	int wait_task;
	int ph_state;
	char rotate;
	int current_shift;
	int dark_dir;
	int dark_val;
};

struct vision_hide_state vhide = {0};

void vision_hide_turn_next()
{
	LLIST_STRUCT task;

	task.dir = vhide.rotate;
	task.speed = 70;
	task.dist = 45;
	task.timeout = 3;
	task.state = &vhide.ph_state;

	tasklist_add_tail(&task);

	vhide.current_shift += task.dist;
	vhide.wait_task = 1;
}

void vision_hide_search_dark(struct v_sframe *sframe, int low_y, int hi_y)
{
	int w = sframe->w / 4;
	int h = sframe->h;
	int i, count;
	int *y = sframe->y;
	uint8_t c0, c1, c2, c3;

	int y_avg = 0, y_max = 0;

	count = ((hi_y - low_y) * w);
	*y += (low_y * w);
	for (i = 0; i < count; i++) {
		c0 = *y & 0xff;
		c1 = (*y >> 8) & 0xff;
		c2 = (*y >> 16) & 0xff;
		c3 = (*y >> 24) & 0xff;
		y_avg += c0 + c1 + c2 + c3;
		y_max |= c0 | c1 | c2 | c3;
	}

	if (vhide.dark_val > y_avg) {
		vhide.dark_val = y_avg;
		vhide.dark_dir = vhide.current_shift;
	}

	printf("vhide.dark_val = %d vhide.dark_dir = %d, y_avg = %d  \n", vhide.dark_val, vhide.dark_dir, y_avg);
	fflush(stdout);
}

void vision_hide_turn_to_dark()
{
	LLIST_STRUCT task;

	task.speed = 70;
	task.timeout = 3;
	task.state = &vhide.ph_state;

	if (vhide.rotate == 'R') {
		task.dir = 'L';
	} else
		task.dir = 'R';

	task.dist = vhide.current_shift - vhide.dark_dir;

	if (task.dist) {
		tasklist_add_tail(&task);
		vhide.wait_task = 1;
	} else vhide.phase++;

}

void vision_hide_search_dark_target(struct v_sframe *sframe, int low_y, int hi_y)
{
	int w = sframe->w;
	int h = sframe->h;
	int cx, cy;
	int *y = sframe->y;
	uint8_t c0, c1, c2, c3;

	int y_max = 0, cx_max = w / 2;

	*y += (low_y * w) / 4;
	for (cy = low_y; cy < hi_y; cy++)
		for (cx = 0; cx < w; cx++) {
			c0 = *y & 0xff;
			c1 = (*y >> 8) & 0xff;
			c2 = (*y >> 16) & 0xff;
			c3 = (*y >> 24) & 0xff;
			if (c0 > y_max) {
				y_max = c0;
				cx_max = cx;
			}
			if (c1 > y_max) {
				y_max = c1;
				cx_max = cx;
			}
			if (c2 > y_max) {
				y_max = c2;
				cx_max = cx;
			}
			if (c3 > y_max) {
				y_max = c3;
				cx_max = cx;
			}
			y++;
		}

	vhide.current_shift = 0;

	if (cx_max < w) {

		cx_max -= w / 2;
		vhide.current_shift = ((cx_max * 100 / (w / 2))*45) / 100;
	}


}

void vision_hide_move_to_dark()
{
	LLIST_STRUCT task;

	if (abs(vhide.current_shift) > 10) {
		if (vhide.current_shift > 0) task.dir = 'r';
		else task.dir = 'l';
		task.speed = 70;
		task.dist = 1000;
		task.timeout = 60;
		task.state = NULL;
	} else {
		task.dir = 'F';
		task.speed = 60;
		task.dist = 1000;
		task.timeout = 60;
		task.state = NULL;
	}

	tasklist_add_tail(&task);

}

void vision_hide_correct_to_dark()
{
	LLIST_STRUCT task;

	if (abs(vhide.current_shift) > 10) {
		if (vhide.current_shift > 0) task.dir = 'r';
		else task.dir = 'l';
		task.speed = 60;
		task.dist = 1000;
		task.timeout = 60;
		task.state = NULL;
	} else {
		task.dir = 'F';
		task.speed = 60;
		task.dist = 1000;
		task.timeout = 60;
		task.state = NULL;
	}

	tasklist_fixed_current_task(0, task.dir, task.speed);
}

void vision_hide_complite()
{
	LLIST_STRUCT task;

	task.dir = vhide.rotate;
	task.speed = 60;
	task.dist = 180;
	task.timeout = 3;
	task.state = NULL;

	tasklist_add_tail(&task);

	vhide.phase = 0xff;
}

void vision_hide_moveback()
{
	LLIST_STRUCT task;

	task.dir = 'B';
	task.speed = 80;
	task.dist = 10;
	task.timeout = 3;
	task.state = NULL;

	tasklist_add_tail(&task);

	vhide.phase = 0xff;
}

void vision_hide_moveback2()
{
	LLIST_STRUCT task;

	task.dir = 'F';
	task.speed = 80;
	task.dist = 10;
	task.timeout = 3;
	task.state = NULL;

	tasklist_add_tail(&task);

	task.dir = 'B';
	task.speed = 80;
	task.dist = 20;
	task.timeout = 3;
	task.state = NULL;

	tasklist_add_tail(&task);

	vhide.phase = 0xff;
}



void vision_hide_init(char rotate, int target_size)
{

	if (target_size < 20) {
		if (target_size&1)
			vhide.phase = VISION_HIDE_BACK;
		else
			vhide.phase = VISION_HIDE_BACK2;
	}
	else vhide.phase = VISION_HIDE_RUN;

	vhide.wait_task = 0;
	vhide.ph_state = 0;
	vhide.rotate = rotate;
	vhide.current_shift = 0;
	vhide.dark_dir = 0;
	vhide.dark_val = 0x7fffffff;
	vision_control_task_stop();
}

int vision_hide(struct v_sframe *sframe, int mode)
{

	if (mode < 0) {
		vision_hide_complite();
		return -1;
	}

	printf("vhide.phase = %d \n", vhide.phase);
	fflush(stdout);

	if (vhide.wait_task) {
		if (vhide.ph_state >= VTASK_COMPLITE) {
			vhide.phase++;
			vhide.wait_task = 0;
		}
		return 0;
	}


	switch (vhide.phase) {
	case VISION_HIDE_RUN:
	case 1:
	case 2:
		vhide.phase++;
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		vision_hide_search_dark(sframe, 0, sframe->h);
		vision_hide_turn_next();
		break;
	case 8:
		vision_hide_search_dark(sframe, 0, sframe->h);
		vision_hide_turn_to_dark();
		break;
	case 9:
		vision_hide_search_dark_target(sframe, 0, sframe->h);
		vision_hide_move_to_dark();
		vhide.phase++;
		break;
	case 10:
		vision_hide_search_dark_target(sframe, 0, sframe->h);
		vision_hide_correct_to_dark();
		break;

	case 11:
		vision_hide_complite();
		return -1;
		break;

	case VISION_HIDE_BACK:
		vision_hide_moveback();
		return -1;
		break;
	case VISION_HIDE_BACK2:
		vision_hide_moveback2();
		return -1;
		break;
	default:
		return -1;
	}

	return vhide.phase;
}
