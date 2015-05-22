/* 
 * File:   vision_hyst.h
 * Author: akrapivniy
 *
 * Created on 15 Январь 2015 г., 21:49
 */

#ifndef VISION_HYST_H
#define	VISION_HYST_H


#define VISION_HISTOGRAM_PIECE 16
#define VISION_HISTOGRAM_SIZE ((VISION_MAX_WIDTH/VISION_HISTOGRAM_PIECE) * (VISION_MAX_HEIGHT/VISION_HISTOGRAM_PIECE))
#define VISION_HISTOGRAM_MAX_WIDTH (VISION_MAX_WIDTH/VISION_HISTOGRAM_PIECE)
#define VISION_HISTOGRAM_NUMS 3

struct histogram8 {
	uint8_t h[8];
};

struct histogram32 {
	uint32_t h[8];
};

struct v_histogram {
	struct histogram8 *u;
	struct histogram8 *v;
	struct histogram8 *y;
	int w, h;
	int res_w, res_h;
};

struct v_histogram_storage {
	struct histogram8 y[VISION_HISTOGRAM_SIZE];
	struct histogram8 u[VISION_HISTOGRAM_SIZE];
	struct histogram8 v[VISION_HISTOGRAM_SIZE];
};


void get_histogram(struct v_frame *frame, struct v_histogram *hframe);


#endif	/* VISION_HYST_H */

