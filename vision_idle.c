#include <stdio.h>
#include <string.h>
#include "vision_core.h"
#include "vision_sframe.h"
#include "vision_floor.h"
#include "vision_control.h"
#include "vision_sframe.h"

struct vision_idle_state {
	int phase;
	int wait_task;
	int ph_state;
	char rotate;
	int current_shift;
	int dist_dir;
	int dist_val;
};

struct vision_idle_state vidle = {0};

void vision_idle_turn_next()
{
	LLIST_STRUCT task;

	task.dir = vidle.rotate;
	task.speed = 60;
	task.dist = 45;
	task.timeout = 3;
	task.state = &vidle.ph_state;

	tasklist_add_tail(&task);

	vidle.current_shift += task.dist;
	vidle.wait_task = 1;
}

void vision_idle_search_floor(struct v_frame *frame)
{
	unsigned int fw = frame->w / 8;
	unsigned int side_wide = ((fw * 30) / 100);
	unsigned int start_center = side_wide;
	unsigned int start_right = side_wide * 2;
	int fdata[VISION_MAX_WIDTH];
	int i, wf = 0;

	memset(fdata, 0, fw * sizeof(int));
	direct_floor_level_min(frame, fdata);

	for (i = start_center; i < start_right; i++) {
		if (fdata[i] < 60)
			fdata[i] = frame->h;
		if (fdata[i] > wf)
			wf = fdata[i];
	}

	if (vidle.dist_val > wf) {
		vidle.dist_val = wf;
		vidle.dist_dir = vidle.current_shift;
	}
	printf("vidle.dist_val = %d vidle.dist_dir = %d wf = %d  \n", vidle.dist_val, vidle.dist_dir, wf);
	fflush(stdout);
}

void vision_idle_turn_to_space()
{
	LLIST_STRUCT task;

	task.speed = 60;
	task.timeout = 3;
	task.state = &vidle.ph_state;

	if (vidle.rotate == 'R') {
		task.dir = 'L';
	} else
		task.dir = 'R';

	task.dist = vidle.current_shift - vidle.dist_dir;

	if (task.dist) {
		tasklist_add_tail(&task);
		vidle.wait_task = 1;
	} else vidle.phase++;
	printf("space: task.dist = %d task.dir = %c \n", task.dist, task.dir);
	fflush(stdout);
}

void vision_idle_movefwd()
{
	LLIST_STRUCT task;

	task.dir = 'F';
	task.speed = 60;
	task.dist = 600;
	task.timeout = 10;
	task.state = NULL;

	tasklist_add_tail(&task);

	vidle.phase = 0xff;
}

void vision_idle_init(char rotate)
{
	vidle.phase = 0;
	vidle.wait_task = 0;
	vidle.ph_state = 0;
	vidle.rotate = rotate;
	vidle.current_shift = 0;
	vidle.dist_val = 0x7fffffff;
	vidle.dist_dir = 45;
}

int vision_idle(struct v_frame *frame, int mode)
{
	printf("vidle.phase = %d \n", vidle.phase);
	fflush(stdout);
	if (vidle.wait_task) {
		if (vidle.ph_state >= VTASK_COMPLITE) {
			vidle.phase++;
			vidle.wait_task = 0;
		}
		return 0;
	}


	switch (vidle.phase) {
	case 0:
	case 1:
	case 2:
		vidle.phase++;
		break;
	case 3:
		vision_idle_search_floor(frame);
		if (vidle.dist_val < 100) {
			vision_idle_movefwd();
			return -1;
		} else vidle.phase++;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		vision_idle_search_floor(frame);
		vision_idle_turn_next();
		break;
	case 13:
		vision_idle_search_floor(frame);
		vision_idle_turn_to_space();
		break;
	default:
		return -1;
	}

	return vidle.phase;
}
