/* 
 * File:   vision_floor.h
 * Author: akrapivniy
 *
 * Created on 29 Декабрь 2014 г., 21:56
 */

#ifndef VISION_FLOOR_H
#define	VISION_FLOOR_H

#include "vision_core.h"
#include "vision_hist.h"

struct vision_floor_color {
	unsigned char min_y, max_y, avg_y;
	unsigned char min_u, max_u, avg_u;
	unsigned char min_v, max_v, avg_v;
};

struct vision_floor_histogram {
	struct histogram8 y[VISION_HISTOGRAM_MAX_WIDTH];
	struct histogram8 u[VISION_HISTOGRAM_MAX_WIDTH];
	struct histogram8 v[VISION_HISTOGRAM_MAX_WIDTH];
};

struct vision_floor_data {
    int w,fw,h;
    int floor_data[VISION_MAX_WIDTH*2];
    int *floor_prev;
    int *floor_current;
    int floor_speed[VISION_MAX_WIDTH];
    int floor_next[VISION_MAX_WIDTH];
    int floor_state[VISION_MAX_WIDTH];
    struct vision_floor_color color;
    struct vision_floor_histogram hist;
};

int vision_init_floor_data (struct vision_floor_data *fd);
int *vision_get_floor_level (struct v_frame *frame, struct vision_floor_data *fd, int floor_delta_threshold);
int vision_get_use_floor (struct vision_floor_data *fd, int wide);
int vision_get_way_floor (struct vision_floor_data *fd);
int direct_floor_color (struct v_frame *frame, int *floor_level, struct vision_floor_histogram *fh, struct vision_floor_color *color);


#endif	/* VISION_FLOOR_H */

