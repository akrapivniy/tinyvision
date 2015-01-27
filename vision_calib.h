
#ifndef VISION_CALIB_H
#define	VISION_CALIB_H

#include "low_control.h"
#include "vision_core.h"
#include "vision_sframe.h"

struct v_calibrate_state {
    int state;
    int full_count;
    int point_count;
    int point_hold;
    int uv_threshold;
    int y_threshold;
    int hx;
    int hy;
    int x;
    int y;
    int x_dir;
    int y_dir;
    int x_max;
    int x_min;
    int y_max;
    int y_min;
    int found_x;
    int found_y;
    int direction;
    int direction_count;
    int direction_loop;
    int w;
    int h;
    int *fire_table;
};

int vision_calibrate_init (struct v_calibrate_state *c, int *table, int w, int h);
int vision_calibrate (struct v_calibrate_state *c, struct v_sframe *his_frame);
int vision_calibrate_finish (struct v_calibrate_state *c);


#endif	/* VISION_CALIB_H */

