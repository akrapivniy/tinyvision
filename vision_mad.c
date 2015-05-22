#include <stdio.h>
#include <string.h>
#include "vision_core.h"
#include "vision_sframe.h"
#include "vision_floor.h"
#include "vision_control.h"
#include "vision_sframe.h"

struct vision_mad_state {
	int phase;
	int wait_task;
	int ph_state;
	char rotate;
	int current_shift;
	int dist_dir;
	int dist_val;
	int timeout;

};

struct vision_mad_state vmad = {0};

void vision_mad_turn_next()
{
	LLIST_STRUCT task;

	task.dir = vmad.rotate;
	task.speed = 80;
	task.dist = 45;
	task.timeout = 3;
	task.state = &vmad.ph_state;

	tasklist_add_tail(&task);

	vmad.current_shift += task.dist;
	vmad.wait_task = 1;
}

void vision_mad_search_floor(struct v_frame *frame)
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
		if (fdata[i] < 30)
			fdata[i] = frame->h;
		if (fdata[i] > wf)
			wf = fdata[i];
	}

	if (vmad.dist_val > wf) {
		vmad.dist_val = wf;
		vmad.dist_dir = vmad.current_shift;
	}
	printf("vmad.dist_val = %d vmad.dist_dir = %d wf = %d  \n", vmad.dist_val, vmad.dist_dir, wf);
	fflush(stdout);
}

void vision_mad_turn_to_space()
{
	LLIST_STRUCT task;

	task.speed = 80;
	task.timeout = 3;
	task.state = &vmad.ph_state;

	if (vmad.rotate == 'R') {
		task.dir = 'L';
	} else
		task.dir = 'R';

	task.dist = vmad.current_shift - vmad.dist_dir;

	if (task.dist) {
		tasklist_add_tail(&task);
		vmad.wait_task = 1;
	} else vmad.phase++;
	printf("space: task.dist = %d task.dir = %c \n", task.dist, task.dir);
	fflush(stdout);
}

void vision_mad_turn(int alert)
{
	LLIST_STRUCT task;

	if (!alert) {
		vmad.phase++;
		return;
	}
	task.dir = alert;
	task.speed = 80;
	task.timeout = 3;
	task.state = &vmad.ph_state;

	task.dist = 45;
	tasklist_add_tail(&task);
	vmad.wait_task = 1;
}

void vision_mad_search_space(struct v_frame *frame)
{
	unsigned int fw = frame->w / 8;
	unsigned int side_wide = ((fw * 30) / 100);
	unsigned int start_center = side_wide;
	unsigned int start_right = side_wide * 2;
	int fdata[VISION_MAX_WIDTH];
	int i, wf = 0, wr = 0, wl = 0;

	memset(fdata, 0, fw * sizeof(int));
	direct_floor_level_min(frame, fdata);

	for (i = 0, wl = 0; i < start_center; i++)
		wl+=fdata[i];

	for (i = start_center, wf = 0; i < start_right; i++)
		wf+=fdata[i];

	for (i = start_right, wr = 0; i < fw; i++)
		wr+=fdata[i];

	
	if ((wr<wl) && (wr<wf)) vmad.rotate = 'r';
	else if ((wl<wr) && (wl<wf)) vmad.rotate = 'l';
	else vmad.rotate = 'F';
}

void vision_mad_move_to_space()
{
	LLIST_STRUCT task;

	task.dir = vmad.rotate;
	task.speed = 80;
	task.dist = 1000;
	task.timeout = 60;
	task.state = NULL;

	tasklist_add_tail(&task);
}

void vision_mad_correct_to_space()
{
	if (tasklist_fixed_current_task(0, vmad.rotate, 80)) 
		vmad.phase = 9;
}

void vision_mad_init(char rotate)
{
	vmad.phase = 0;
	vmad.wait_task = 0;
	vmad.ph_state = 0;
	vmad.rotate = rotate;
	vmad.current_shift = 0;
	vmad.dist_val = 0x7fffffff;
	vmad.dist_dir = 45;
	vmad.timeout = 0;
}

int vision_mad(struct v_frame *frame, int alert)
{
	printf("vmad.phase = %d \n", vmad.phase);
	fflush(stdout);
	vmad.timeout++;
	if (vmad.timeout > 1000)
		return -1;

	if (vmad.wait_task) {
		if (vmad.ph_state >= VTASK_COMPLITE) {
			vmad.phase++;
			vmad.wait_task = 0;
		}
		return 0;
	}

	if (alert && vmad.phase>9)
		vmad.phase = 9;

	switch (vmad.phase) {
	case 0:
	case 1:
	case 2:
		vmad.phase++;
		break;
	case 3:
	case 4:
	case 6:
	case 7:
		vision_mad_search_floor(frame);
		vision_mad_turn_next();
		break;
	case 8:
		vision_mad_search_floor(frame);
		vision_mad_turn_to_space();
		break;
	case 9:
		vision_mad_turn(alert);
		break;
	case 10:
		vision_mad_search_space(frame);
		vision_mad_move_to_space();
		vmad.phase++;
		break;
	case 11:
		vision_mad_search_space(frame);
		vision_mad_correct_to_space();
		break;
	default:
	case 12:
		vmad.phase = 9;
		break;
	}

	return vmad.phase;
}
