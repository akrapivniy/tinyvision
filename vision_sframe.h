/* 
 * File:   vision_sframe.h
 * Author: akrapivniy
 *
 * Created on 15 Январь 2015 г., 21:30
 */

#ifndef VISION_SFRAME_H
#define	VISION_SFRAME_H

#include "vision_core.h"

#define VISION_SFRAME_PIECE 16
#define VISION_SFRAME_SIZE ((VISION_MAX_WIDTH/VISION_SFRAME_PIECE) * (VISION_MAX_HEIGHT/VISION_SFRAME_PIECE))
#define VISION_SFRAME_NUMS 4

struct v_sframe_storage {
	int y[VISION_SFRAME_SIZE];
	int u[VISION_SFRAME_SIZE];
	int v[VISION_SFRAME_SIZE];
};

struct v_sframe {
	int *u;
	int *v;
	int *y;
	int w, h;
	int res_w, res_h;
};

inline void set_sframe_storage(struct v_sframe *f, struct v_sframe_storage *s);
inline void exchange_storage(struct v_sframe *f1, struct v_sframe *f2);
inline void copy_storage(struct v_sframe *dst, struct v_sframe *src);
int copy_avg_sframe(struct v_sframe *dst, struct v_sframe *src,
	int uv_act_threshold, int y_act_threshold, int uv_pass_threshold, int y_pass_threshold);
int compare_sframe(struct v_sframe *f1, struct v_sframe *f2,
	unsigned int *uvdiff, unsigned int *ydiff,
	unsigned int uv_threshold, unsigned int y_threshold);
void get_sframe(struct v_frame *frame, struct v_sframe *hframe);



#endif	/* VISION_SFRAME_H */

