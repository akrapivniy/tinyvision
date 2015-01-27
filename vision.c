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
#include "vision.h"


struct v_state {
    int tune_background;
    int use_background;
    int use_dyn_background;
    int stable_background;
   
    int save_floor_color;
    int use_floor_color;
    struct vision_floor_data floor_data;

    int calibrate_mode;
    int lock_target_mode;
    int table_state;
};



struct v_config {
    int use_background;
};

struct v_sframe_storage *sframe[VISION_SFRAME_NUMS];
struct v_sframe sf_dyn_background;
struct v_sframe sf_background;
struct v_sframe sf_frame;
struct v_histogram_storage hist_frame;

struct v_state vstate;
struct v_config vconfig;


void print_hist (struct v_histogram *hf)
{
	int x ,y, i;

	printf ("\n");
	for (y = 0; y < hf->h; y++ ) {
		for (x = 0; x < hf->w; x++ ) {
			i = y*hf->w+x;
			printf ("(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x)-",
				hf->y[i].h[0],hf->y[i].h[1],hf->y[i].h[2],hf->y[i].h[3],hf->y[i].h[4],hf->y[i].h[5],hf->y[i].h[6],hf->y[i].h[7],
				hf->v[i].h[0],hf->v[i].h[1],hf->v[i].h[2],hf->v[i].h[3],hf->v[i].h[4],hf->v[i].h[5],hf->v[i].h[6],hf->v[i].h[7],
				hf->u[i].h[0],hf->u[i].h[1],hf->u[i].h[2],hf->u[i].h[3],hf->u[i].h[4],hf->u[i].h[5],hf->u[i].h[6],hf->u[i].h[7]);
		}
		printf ("\n");
	}
}


void print_sframe (struct v_sframe *hf)
{
	int x ,y, i;

	printf ("\n");
	for (y = 0; y < hf->h; y++ ) {
		for (x = 0; x < hf->w; x++ ) {
			i = y*hf->w+x;
			printf ("%02x:%02x:%02x-",hf->y[i],hf->v[i],hf->u[i]);
		}
		printf ("\n");
	}
}

void print_diff_sframe (struct v_sframe *hf, struct v_sframe *hf2)
{
	int x ,y, i;

	printf ("\n");
	for (y = 0; y < hf->h; y++ ) {
		for (x = 0; x < hf->w; x++ ) {
			i = y*hf->w+x;
			printf ("%02x:%02x:%02x-",	abs(hf->y[i] - hf2->y[i]),
							abs(hf->v[i] - hf2->v[i]),
							abs(hf->u[i] - hf2->u[i]));
		}
		printf ("\n");
	}
}

int vision_load_config ()
{
	int fd;
	fd = open("/tmp/vision.config", O_RDONLY);
	if (fd < 0) return -1;
	read (fd, &vconfig, sizeof (struct v_config));
	close(fd);
	printf ("Load config from file\n");
	return 0;
}

int vision_save_config ()
{
	int fd;
	fd = open("/tmp/vision.config", O_WRONLY|O_CREAT);
	if (fd < 0) return -1;
	write(fd, &vconfig, sizeof (struct v_config));
	close(fd);
	printf ("Save config to file\n");
	return 0;
}


int init_vision ()
{
    int i;
    for (i = 0; i < VISION_SFRAME_NUMS; i++)
	sframe[i] = malloc(sizeof (struct v_sframe_storage));

    set_sframe_storage (&sf_background, sframe[0]);
    set_sframe_storage (&sf_dyn_background, sframe[1]);
    set_sframe_storage (&sf_frame, sframe[2]);

    vstate.tune_background = 60;
    vstate.use_background = 0;
    vstate.use_dyn_background = 0;

    vstate.calibrate_mode = 0;
    vstate.lock_target_mode = 1;
    vision_init_floor_data (&vstate.floor_data);
//    if (vision_load_config ())
//		memset (&vision_config, 0, sizeof (struct v_config));
    return 0;
}


void vision_tunebackground (struct v_frame *frame)
{
	unsigned count, uvdiff, ydiff;

        if (!vstate.use_background) {
		get_sframe (frame, &sf_background);
		vstate.use_background = 1;
		return;
	} 
        if (!vstate.use_dyn_background) {
		get_sframe (frame, &sf_dyn_background);
		vstate.use_dyn_background = 10;
		return;
	}
	vstate.stable_background = 0;
	vstate.tune_background --;
	get_sframe (frame, &sf_frame);
	count = compare_sframe (&sf_frame, &sf_dyn_background,  &uvdiff , &ydiff,5 ,20);

	if ((uvdiff<2)&&(ydiff<5)&&(count<((sf_frame.w*sf_frame.h)/2))) 
	{
	    if (vstate.use_dyn_background == 1) {
			copy_storage (&sf_background, &sf_dyn_background);
			vstate.tune_background = 0;
			vstate.stable_background = 1;
			return;
			}
	    printf ("+");fflush (stdout);
	    vstate.use_dyn_background --;
	    return;
	} else {
	    vstate.use_dyn_background = 20;
	    exchange_storage (&sf_dyn_background,&sf_frame);
	    printf ("-");fflush (stdout);
	}
}

void vision_update_background (struct v_sframe *his_frame)
{
	copy_avg_sframe (&sf_dyn_background, his_frame, 5, 10, 4, 5);
}

void vision_frame (struct v_frame *frame)
{
    int ret;
    int i, dist, way, fl_th;
    char dir, speed;
    LLIST_STRUCT f,f2;

   if (vstate.tune_background) {
	vision_tunebackground (frame);
	printf ("Tuning background - %u \n",vstate.tune_background); fflush(stdout);
	return;
    }
//    get_histogram (frame, &hframe);

/*   if (vstate.use_floor_color) {
    direct_hyst_floor_level_min (&hframe, &vstate.floor_data.hist,fl);
   } else 
 */

    vision_get_floor_level (frame,  &vstate.floor_data, 3);

/*
    for (i=0;i<frame->w/8;i++)
    {
	printf (" (%3d-%3d)",vstate.floor_data.floor_current[i],fl[i]);	 
    }
	printf ("\n");


   if (vstate.save_floor_color) {
	get_control_state (&dir, &speed);
	if (dir == 'S')  {
		direct_floor_color (frame, vstate.floor_data.floor_current, &vstate.floor_data.hist, &vstate.floor_data.color);
		vstate.use_floor_color = 1;
	} else {
				
	}
   }    
*/	
	
/*    
    hf = &vision_state.floor_data.hist;
    for (i = 0; i < frame->w/16; i++ ) {
		printf ("(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x):(%02x%02x%02x%02x%02x%02x%02x%02x)-",
			hf->y[i].h[0],hf->y[i].h[1],hf->y[i].h[2],hf->y[i].h[3],hf->y[i].h[4],hf->y[i].h[5],hf->y[i].h[6],hf->y[i].h[7],
			hf->v[i].h[0],hf->v[i].h[1],hf->v[i].h[2],hf->v[i].h[3],hf->v[i].h[4],hf->v[i].h[5],hf->v[i].h[6],hf->v[i].h[7],
			hf->u[i].h[0],hf->u[i].h[1],hf->u[i].h[2],hf->u[i].h[3],hf->u[i].h[4],hf->u[i].h[5],hf->u[i].h[6],hf->u[i].h[7]);
    }   
	printf ("\n");

    vision_get_floor_level (frame,  &vision_state.floor_data, 3);

    printf ("floor color y = (%d [%d:%d]) v = (%d [%d:%d]) u = (%d [%d:%d]) \n", 
		vision_state.floor_data.color.avg_y, vision_state.floor_data.color.min_y, vision_state.floor_data.color.max_y,
		vision_state.floor_data.color.avg_v, vision_state.floor_data.color.min_v, vision_state.floor_data.color.max_v,
		vision_state.floor_data.color.avg_u, vision_state.floor_data.color.min_u, vision_state.floor_data.color.max_u);
    
    for (i=0;i<frame->w/8;i++)
    {
	printf (" (%3d-%3d)",vision_state.floor_data.floor_current[i],fl[i]);	 
    }

    printf (" dist = %d\n",dist);
*/
}
