#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "low_control.h"
#include "vision_calib.h"
#include "vision_core.h"
#include "vision_floor.h"
#include "vision_sframe.h"
#include "vision_hist.h"
#include "vision_control.h"
#include "vision_map.h"
#include "vision_hide.h"
#include "vision_target.h"
#include "vision.h"
#include "vision_debug.h"

#define VISION_MODE_BACKGROUND	 (1<<0)
#define VISION_MODE_HIDE	 (1<<1)
#define VISION_MODE_USERCONTROL	 (1<<2)
#define VISION_MODE_TESTWALL	 (1<<3)
#define VISION_MODE_FWDWALL	 (1<<4)
#define VISION_MODE_IDLE	 (1<<5)
#define VISION_MODE_MAD		 (1<<6)
#define VISION_MODE_FOLLOWME	 (1<<6)

#define VISION_SET_MODE(var,mode) var |= mode;
#define VISION_RESET_MODE(var,mode) var &= ~mode;

struct v_state {
	uint32_t mode;

	int tune_background;
	int use_background;
	int use_dyn_background;

	int test_wall_timeout;
	int test_wall_state;

	int fwd_wall_timeout;
	int fwd_wall_state;

	int check_background_state;

	int idle_timeout;

	int mad_timeout;

	int save_floor_color;
	int use_floor_color;
	struct vision_floor_data floor_data;

	int calibrate_mode;
	int lock_target_mode;
	int table_state;
	int old_direction;
};

struct v_config {
	int use_background;
};

struct v_sframe_storage *sframe[VISION_SFRAME_NUMS];
struct v_sframe sf_dyn_background;
struct v_sframe sf_lastframe;
struct v_sframe sf_background;
struct v_sframe sf_frame;
struct v_histogram_storage hist_frame;

struct v_state vstate;
struct v_config vconfig;

void print_hist(struct v_histogram *hf)
{
	int x, y, i;

	printf("\n");
	for (y = 0; y < hf->h; y++) {
		for (x = 0; x < hf->w; x++) {
			i = y * hf->w + x;
			printf("(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x)-",
				hf->y[i].h[0], hf->y[i].h[1], hf->y[i].h[2], hf->y[i].h[3], hf->y[i].h[4], hf->y[i].h[5], hf->y[i].h[6], hf->y[i].h[7],
				hf->v[i].h[0], hf->v[i].h[1], hf->v[i].h[2], hf->v[i].h[3], hf->v[i].h[4], hf->v[i].h[5], hf->v[i].h[6], hf->v[i].h[7],
				hf->u[i].h[0], hf->u[i].h[1], hf->u[i].h[2], hf->u[i].h[3], hf->u[i].h[4], hf->u[i].h[5], hf->u[i].h[6], hf->u[i].h[7]);
		}
		printf("\n");
	}
}

void print_sframe(struct v_sframe *hf)
{
	int x, y, i;

	printf("\n");
	for (y = 0; y < hf->h; y++) {
		for (x = 0; x < hf->w; x++) {
			i = y * hf->w + x;
			printf("%02x:%02x:%02x-", hf->y[i], hf->v[i], hf->u[i]);
		}
		printf("\n");
	}
}

void print_diff_sframe(struct v_sframe *hf, struct v_sframe *hf2)
{
	int x, y, i;

	printf("\n");
	for (y = 0; y < hf->h; y++) {
		for (x = 0; x < hf->w; x++) {
			i = y * hf->w + x;
			printf("%02x:%02x:%02x-", abs(hf->y[i] - hf2->y[i]),
				abs(hf->v[i] - hf2->v[i]),
				abs(hf->u[i] - hf2->u[i]));
		}
		printf("\n");
	}
}

int vision_load_config()
{
	int fd;
	fd = open("/tmp/vision.config", O_RDONLY);
	if (fd < 0) return -1;
	read(fd, &vconfig, sizeof(struct v_config));
	close(fd);
	debug_init("Load config from file\n");
	return 0;
}

int vision_save_config()
{
	int fd;
	fd = open("/tmp/vision.config", O_WRONLY | O_CREAT);
	if (fd < 0) return -1;
	write(fd, &vconfig, sizeof(struct v_config));
	close(fd);
	debug_init("Save config to file\n");
	return 0;
}

int init_vision()
{
	int i;

	vision_core_init();

	for (i = 0; i < VISION_SFRAME_NUMS; i++)
		sframe[i] = malloc(sizeof(struct v_sframe_storage));

	set_sframe_storage(&sf_background, sframe[0]);
	set_sframe_storage(&sf_dyn_background, sframe[1]);
	set_sframe_storage(&sf_lastframe, sframe[2]);
	set_sframe_storage(&sf_frame, sframe[3]);

	vstate.tune_background = 60;
	vstate.use_background = 0;
	vstate.use_dyn_background = 0;

	vstate.old_direction = 'S';
	vstate.mode = 0;

	vision_mode_background_init();
	vision_init_floor_data(&vstate.floor_data);
	//    if (vision_load_config ())
	//		memset (&vision_config, 0, sizeof (struct v_config));

	vision_map_init();
	return 0;
}

void vision_tunebackground(struct v_sframe *sframe)
{
	unsigned count, uvdiff, ydiff;

	if (!vstate.use_background) {
		copy_storage(&sf_background, sframe);
		vstate.use_background = 1;
		return;
	}
	if (!vstate.use_dyn_background) {
		copy_storage(&sf_dyn_background, sframe);
		vstate.use_dyn_background = 10;
		return;
	}

	vstate.tune_background--;

	count = compare_sframe(sframe, &sf_dyn_background, &uvdiff, &ydiff, 5, 20);

	debug_process("count = %d", count);

	if ((uvdiff < 2)&&(ydiff < 5)&&(count < ((sframe->w * sframe->h) / 2))) {
		if (vstate.use_dyn_background == 1) {
			copy_storage(&sf_background, &sf_dyn_background);
			vstate.tune_background = 0;
			return;
		}
		vstate.use_dyn_background--;
		return;
	} else {
		vstate.use_dyn_background = 20;
		exchange_storage(&sf_dyn_background, sframe);
	}
}

void vision_update_background(struct v_sframe *his_frame)
{
	copy_avg_sframe(&sf_dyn_background, his_frame, 5, 10, 4, 5);
}

void vision_emergence_stop()
{

}

void vision_check_obstructions(struct v_frame *frame, uint32_t *wl, uint32_t *wf, uint32_t *wr)
{
	vision_get_floor_level(frame, &vstate.floor_data, 3);
	vision_get_wall_from_floor(&vstate.floor_data, wl, wf, wr);

	debug_process("distance [%d:%d:%d]", *wl, *wf, *wr);
	if (*wl > (vstate.floor_data.h - 2))
		*wl = 1;
	else *wl = 0;
	if (*wf > (vstate.floor_data.h - 2))
		*wf = 1;
	else *wf = 0;
	if (*wr > (vstate.floor_data.h - 2))
		*wr = 1;
	else *wr = 0;
	vision_map_set_near_wall(*wl, *wf, *wr);

}

void vision_check_background(struct v_sframe *sframe)
{
	unsigned count, uvdiff, ydiff;

	count = compare_sframe(sframe, &sf_lastframe, &uvdiff, &ydiff, 2, 5);
	copy_storage(&sf_lastframe, sframe);
	debug_process("count=%d, uvdiff=%d,  ydiff=%d", count, uvdiff, ydiff);

	if (count < 5)
		vstate.check_background_state++;
	else
		vstate.check_background_state = 0;


	if (vstate.check_background_state > 5) {
		vision_control_task_dangerous();
		VISION_SET_MODE(vstate.mode, VISION_MODE_FWDWALL);
		debug_process("ALERT!");
	}

}

void vision_mode_test_obstructions_init()
{
	debug_process("start");

	if (vstate.test_wall_timeout) {
		vstate.test_wall_timeout--;
		return;
	}

	VISION_SET_MODE(vstate.mode, VISION_MODE_TESTWALL);
	vision_control_task_pause();
	vstate.test_wall_timeout = 5;
	vstate.test_wall_state = 0;
}

void vision_mode_test_obstructions(struct v_frame *frame)
{
	uint32_t wl, wf, wr;

	debug_process("test_wall_timeout =%d ", vstate.test_wall_timeout);
	if (vstate.test_wall_timeout == 0) {
		if (vstate.test_wall_state > 0) {
			vision_control_task_restore();
		} else {
			vision_control_task_dangerous();
			VISION_SET_MODE(vstate.mode, VISION_MODE_FWDWALL);
		}
		VISION_RESET_MODE(vstate.mode, VISION_MODE_TESTWALL);
		return;
	} else
		vstate.test_wall_timeout--;

	vision_check_obstructions(frame, &wl, &wf, &wr);

	if (wf) vstate.test_wall_state--;
	else vstate.test_wall_state++;

}

void vision_mode_hide_init(struct v_frame *frame, int target_size)
{
	int ret;
	VISION_SET_MODE(vstate.mode, VISION_MODE_HIDE);

	vision_get_floor_level(frame, &vstate.floor_data, 3);
	ret = vision_get_way_floor(&vstate.floor_data);

	if (ret > 0) vision_hide_init('L', target_size);
	else vision_hide_init('R', target_size);


}

void vision_mode_hide(struct v_sframe *sframe)
{
	int ret;

	if (vstate.mode & VISION_MODE_FWDWALL) {
		vision_hide(sframe, -1);
		VISION_RESET_MODE(vstate.mode, VISION_MODE_HIDE);
	}

	if (vstate.mode & VISION_MODE_TESTWALL) return;

	ret = vision_hide(sframe, 0);
	if (ret < 0)
		VISION_RESET_MODE(vstate.mode, VISION_MODE_HIDE);

}

void vision_mode_hide_close(struct v_sframe *sframe)
{
	vision_hide(sframe, -1);
	VISION_RESET_MODE(vstate.mode, VISION_MODE_HIDE);
}

void vision_mode_idle_init(struct v_frame *frame)
{
	int ret;
	VISION_SET_MODE(vstate.mode, VISION_MODE_IDLE);

	vision_get_floor_level(frame, &vstate.floor_data, 3);
	ret = vision_get_way_floor(&vstate.floor_data);

	if (ret > 0) vision_idle_init('L');
	else vision_idle_init('R');

	vstate.idle_timeout = 0;
}

void vision_mode_idle(struct v_frame *frame)
{
	int ret;

	ret = vision_idle(frame, 0);
	if (ret < 0)
		VISION_RESET_MODE(vstate.mode, VISION_MODE_IDLE);

}

void vision_mode_mad_init(struct v_frame *frame)
{
	int ret;
	
	debug_process ("mad mode init");
	VISION_SET_MODE(vstate.mode, VISION_MODE_MAD);

	vision_get_floor_level(frame, &vstate.floor_data, 3);
	ret = vision_get_way_floor(&vstate.floor_data);

	if (ret > 0) vision_mad_init('L');
	else vision_mad_init('R');

	vstate.mad_timeout = 0;
}

void vision_mode_mad(struct v_frame *frame)
{
	int ret;
	int alert = 0;
	
	debug_process ("mad mode control");
	
	if (vstate.mode & VISION_MODE_FWDWALL) {
		vision_get_floor_level(frame, &vstate.floor_data, 3);
		ret = vision_get_way_floor(&vstate.floor_data);

		if (ret > 0) alert = 'L';
		else alert = 'R';
	}
	
	ret = vision_mad(frame, alert);
	if (ret < 0)
		VISION_RESET_MODE(vstate.mode, VISION_MODE_IDLE);

}

void vision_mode_background_init()
{
	VISION_SET_MODE(vstate.mode, VISION_MODE_BACKGROUND);
	vstate.tune_background = 60;
}

void vision_mode_background()
{
	vision_tunebackground(&sf_frame);
	if (vstate.tune_background == 0)
		VISION_RESET_MODE(vstate.mode, VISION_MODE_BACKGROUND);
}

void vision_rotate_from_wall(struct v_frame *frame)
{
	int ret;
	LLIST_STRUCT task;

	if (vstate.fwd_wall_timeout)
		vstate.fwd_wall_state++;
	else vstate.fwd_wall_state = 0;

	task.speed = 60;
	task.timeout = 3;
	task.state = NULL;
	task.dist = 90;

	vision_get_floor_level(frame, &vstate.floor_data, 3);
	ret = vision_get_way_floor(&vstate.floor_data);
	if (ret > 0) task.dir = 'L';
	else if (ret < 0) task.dir = 'R';
	else {
		task.dist = 180;
		task.dir = 'L';
	}

	if (vstate.fwd_wall_state) {
		if (vstate.fwd_wall_state & 1) {
			if (vstate.fwd_wall_state & 2) task.dir = 'L';
			else task.dir = 'R';
			task.dist = 180;
		} else {
			task.dir = 'B';
			task.dist = 10;
		}
	}

	tasklist_add_tail(&task);

	vstate.fwd_wall_timeout = 5;
}

void vision_frame(struct v_frame *frame)
{
	char dir, speed;
	uint32_t wl, wf, wr;
	uint32_t target_size;

	get_control_state(&dir, &speed);

	get_sframe(frame, &sf_frame);

	debug_process("[mode = %x; dir = %c]", vstate.mode, dir);
	fflush(stdout);
	switch (dir) {

	case 'L':
	case 'R':
	case 'B':
		VISION_RESET_MODE(vstate.mode, VISION_MODE_FWDWALL);
		vision_check_background(&sf_frame);
		break;

	case 'F':
	case 'l':
	case 'r':
		vision_check_background(&sf_frame);
		vision_check_obstructions(frame, &wl, &wf, &wr);

		if (wf) vision_mode_test_obstructions_init();
		else vstate.test_wall_timeout = 1;
		break;

	case 'S':
	default:
		if (vstate.mode & VISION_MODE_TESTWALL) {
			vision_mode_test_obstructions(frame);

		}

		if (vstate.mode & VISION_MODE_BACKGROUND) {
			vision_mode_background();
		}

		if (vstate.old_direction != 'S') {
			vision_mode_background_init();
			vstate.idle_timeout = 0;
		}

		if (vstate.mode == 0) {
			target_size = vision_check_targets(&sf_frame, &sf_background);
			if (target_size)
				vision_mode_hide_init(frame, target_size);
		}
		if ((vstate.mode == 0) && (vstate.idle_timeout > 80)) {
			vision_mode_idle_init(frame);
		} else vstate.idle_timeout++;

		if ((vstate.mode == 0) && (vstate.mad_timeout > 600)) {
			vision_mode_mad_init(frame);
		} else vstate.mad_timeout++;
		break;
	};


	if (vstate.mode & VISION_MODE_FWDWALL) {
		vision_rotate_from_wall(frame);
	} else {
		if (vstate.fwd_wall_timeout)
			vstate.fwd_wall_timeout--;
	}

	if (vstate.mode & VISION_MODE_HIDE) {
		vision_mode_hide(&sf_frame);
	}

	if (vstate.mode & VISION_MODE_IDLE) {
		vision_mode_idle(frame);
	}

	if (vstate.mode & VISION_MODE_MAD) {
		vision_mode_mad(frame);
	}

	vstate.old_direction = dir;
}
