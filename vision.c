#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "fire.h"
#include "vision_calib.h"
#include "vision_core.h"
#include "vision.h"


struct v_histogram_storage {
    int y[VISION_HISTOGRAM_SIZE];
    int u[VISION_HISTOGRAM_SIZE];
    int v[VISION_HISTOGRAM_SIZE];
};

struct v_state {
    int use_background;
    int use_dyn_background;
    int tune_background;
    int stable_background;
    int calibrate_mode;
    int lock_target_mode;

    int table_state;
    int fire_table[VISION_HISTOGRAM_SIZE];
};


struct v_histogram_storage *histogram[VISION_HISTOGRAM_NUMS];

struct v_histogram_frame his_background;
struct v_histogram_frame his_dyn_background;
struct v_histogram_frame his_frame;
struct v_state vision_state;
struct v_target_lock main_target;
struct v_calibrate_state calibrate;


inline void set_his_storage (struct v_histogram_frame *f, struct v_histogram_storage *s)
{
    f->u = s->u;
    f->v = s->v;
    f->y = s->y;
}

inline void exchange_storage (struct v_histogram_frame *f1, struct v_histogram_frame *f2)
{
    int *y,*u,*v;
    u = f1->u;     v = f1->v;     y = f1->y;
    f1->u = f2->u; f1->v = f2->v; f1->y = f2->y;
    f2->u = u;     f2->v = v;     f2->y = y;
}


int vision_calibrate_init (struct v_calibrate_state *c, int *table, int w, int h);

int vision_load_fire_table ()
{
	int fd;
	fd = open("/usr/share/fire.tbl", O_RDONLY);
	if (fd < 0) return -1;
	read (fd, vision_state.fire_table, VISION_HISTOGRAM_SIZE*sizeof (int));
	close(fd);
	printf ("Load fire table from file\n");
	return 0;
}

int vision_save_fire_table ()
{
	int fd;
	fd = open("/usr/share/fire.tbl", O_WRONLY|O_CREAT);
	if (fd < 0) return -1;
	write(fd, vision_state.fire_table, VISION_HISTOGRAM_SIZE*sizeof (int));
	close(fd);
	printf ("Save fire table to file\n");
	return 0;
}


int init_vision ()
{
    int i;
    
    for (i = 0; i < VISION_HISTOGRAM_NUMS; i++)
	histogram[i] = malloc(sizeof (struct v_histogram_storage));
    set_his_storage (&his_background, histogram[0]);
    set_his_storage (&his_dyn_background, histogram[1]);
    set_his_storage (&his_frame, histogram[2]);
    vision_state.tune_background = 60;
    vision_state.use_background = 0;
    vision_state.use_dyn_background = 0;
    memset (&main_target, 0, sizeof (struct v_target_lock));
    vision_state.calibrate_mode = 0;
    vision_state.lock_target_mode = 1;
//    if (vision_load_fire_table ())
//    memset (vision_state.fire_table, 0,VISION_HISTOGRAM_SIZE);
//    vision_calibrate_init (&calibrate, vision_state.fire_table, 640/16,480/16);
    return 0;
}


void vision_tunebackground (struct v_frame *frame)
{
	unsigned count, uvdiff, ydiff;

        if (!vision_state.use_background) {
		get_his_frame (frame, &his_background);
		vision_state.use_background = 1;
		return;
	} 
        if (!vision_state.use_dyn_background) {
		get_his_frame (frame, &his_dyn_background);
		vision_state.use_dyn_background = 10;
		return;
	}
	vision_state.stable_background = 0;
	vision_state.tune_background --;
	get_his_frame (frame, &his_frame);
	count = compare_histogram (&his_frame, &his_dyn_background,  &uvdiff , &ydiff,5 ,20);

	if ((uvdiff<2)&&(ydiff<5)&&(count<((his_frame.w*his_frame.h)/2))) 
	{
	    if (vision_state.use_dyn_background == 1) {
			exchange_storage (&his_dyn_background,&his_background);
			vision_state.tune_background = 0;
			vision_state.use_dyn_background = 0;
			vision_state.stable_background = 1;
			return;
			}
	    printf ("+");fflush (stdout);
	    vision_state.use_dyn_background --;
	    return;
	} else {
	    vision_state.use_dyn_background = 20;
	    exchange_storage (&his_dyn_background,&his_frame);
	    printf ("-");fflush (stdout);
	}
}


int vision_lock_target (struct v_histogram_frame *his_frame)
{
    struct v_target targets[100];
    unsigned int target_count;
    int target_map[VISION_HISTOGRAM_SIZE];
    int tx = 0, ty = 0, tab;
    static int fx = 0, fy = 0;

    target_count = get_targets (his_frame, &his_background, target_map, targets, 5, 10);
    
    if (target_count == 0) {
	    fire (fx,fy, 0);
	    return -1;
    }
    if (!find_target (targets, target_count, &main_target, &tx, &ty))
		find_best_target (targets, target_count, &main_target, &tx, &ty);

  /*  tab = vision_state.fire_table[ty*40+tx];
    if (tab) {
	    fx = tab>>8;
	    fy = tab&0xff;
    } else {*/
    fx = 60+(120/his_frame->w)*tx-10;
    fy = 145-(75/his_frame->h)*ty+15;
    
    
    fire (fx, fy, 1);
    return 0;
}




void vision_frame (struct v_frame *frame)
{
    int ret;

    if (vision_state.tune_background) {
	vision_tunebackground (frame);
	printf ("Tuning background - %u \n",vision_state.tune_background); fflush(stdout);
	return;
    }
    get_his_frame (frame, &his_frame);

    if (vision_state.calibrate_mode) {
	ret = vision_calibrate (&calibrate, &his_frame);
	printf ("Calibrate loop - %d \n",ret); fflush(stdout);
	if (ret > 0) {
		vision_calibrate_finish (&calibrate);
		vision_save_fire_table ();
		vision_state.calibrate_mode = 0;
	}
	return;
    }
    
    if (vision_state.lock_target_mode)
		vision_lock_target (&his_frame);

}
