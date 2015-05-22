#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_core.h"
#include "vision_sframe.h"

inline void set_sframe_storage(struct v_sframe *f, struct v_sframe_storage *s)
{
	f->u = s->u;
	f->v = s->v;
	f->y = s->y;
}

inline void exchange_storage(struct v_sframe *f1, struct v_sframe *f2)
{
	int *y, *u, *v;
	u = f1->u;
	v = f1->v;
	y = f1->y;
	f1->u = f2->u;
	f1->v = f2->v;
	f1->y = f2->y;
	f2->u = u;
	f2->v = v;
	f2->y = y;
}

inline void copy_storage(struct v_sframe *dst, struct v_sframe *src)
{
	memcpy(dst->u, src->u, VISION_SFRAME_SIZE * sizeof(int));
	memcpy(dst->v, src->v, VISION_SFRAME_SIZE * sizeof(int));
	memcpy(dst->y, src->y, VISION_SFRAME_SIZE * sizeof(int));
	dst->h = src->h;
	dst->w = src->w;
	dst->res_h = src->res_h;
	dst->res_w = src->res_w;
}

int compare_sframe(struct v_sframe *f1, struct v_sframe *f2,
	unsigned int *uvdiff, unsigned int *ydiff,
	unsigned int uv_threshold, unsigned int y_threshold)
{
	unsigned int i, count = 0, uvd = 0, yd = 0;
	unsigned int fsize;
	int *hx1;
	int *hx2;
	unsigned int diff;

	fsize = f1->w * f1->h; //check f2


	hx1 = f1->u;
	hx2 = f2->u;
	for (i = 0; i < fsize; i++) {
		diff = abs(hx1[i] - hx2[i]);
		uvd += diff;
		if (diff > uv_threshold) count++;
	}

	hx1 = f1->v;
	hx2 = f2->v;
	for (i = 0; i < fsize; i++) {
		diff = abs(hx1[i] - hx2[i]);
		uvd += diff;
		if (diff > uv_threshold) count++;
	}
	*uvdiff = uvd / (fsize * 2);

	hx1 = f1->y;
	hx2 = f2->y;
	for (i = 0; i < fsize; i++) {
		diff = abs(hx1[i] - hx2[i]);
		yd += diff;
		if (diff > y_threshold) count++;
	}
	*ydiff = yd / fsize;
	return count;
}

int copy_avg_sframe(struct v_sframe *dst, struct v_sframe *src,
	int uv_act_threshold, int y_act_threshold, int uv_pass_threshold, int y_pass_threshold)
{
	unsigned int i;
	unsigned int fsize;
	int *hsrc_u = src->u;
	int *hsrc_v = src->v;
	int *hsrc_y = src->y;
	int *hdst_u = dst->u;
	int *hdst_v = dst->v;
	int *hdst_y = dst->y;
	int diff_u, diff_y, diff_v;
	int new_u, new_y, new_v;

	fsize = src->w * src->h; //check dst

	for (i = 0; i < fsize; i++) {
		diff_u = abs(hsrc_u[i] - hdst_u[i]);
		diff_v = abs(hsrc_v[i] - hdst_v[i]);
		diff_y = abs(hsrc_y[i] - hdst_y[i]);

		if ((diff_u < uv_act_threshold)&&
			(diff_v < uv_pass_threshold)&&
			(diff_y < y_pass_threshold))
			new_u = hdst_u[i] + ((hsrc_u[i] - hdst_u[i]) / 2);
		else new_u = hdst_u[i];

		if ((diff_u < uv_pass_threshold)&&
			(diff_v < uv_act_threshold)&&
			(diff_y < y_pass_threshold))
			new_v = hdst_v[i] + ((hsrc_v[i] - hdst_v[i]) / 2);
		else new_v = hdst_v[i];

		if ((diff_u < uv_pass_threshold)&&
			(diff_v < uv_pass_threshold)&&
			(diff_y < y_act_threshold))
			new_y = hdst_y[i] + ((hsrc_y[i] - hdst_y[i]) / 2);
		else new_y = hdst_y[i];
		hdst_u[i] = new_u;
		hdst_v[i] = new_v;
		hdst_y[i] = new_y;
	}

	return 0;
}

void get_sframe(struct v_frame *frame, struct v_sframe *hframe)
{
	unsigned int w = frame->w;
	unsigned int h = frame->h;
	unsigned int pixel_size = (w * h);
	unsigned int y_size = pixel_size >> 2;
	unsigned int u_size = pixel_size >> 3;
	//    unsigned int v_size = u_size;
	unsigned int *y = frame->pixmap;
	unsigned int *u = y + y_size;
	unsigned int *v = u + u_size;
	int *gu = hframe->u;
	int *gv = hframe->v;
	int *gy = hframe->y;
	unsigned int *_gy;
	int *_gu, *_gv;
	unsigned int gmx = w >> 4; // every  16x16 pixel
	unsigned int gmy = h >> 4; // every  16x16 pixel
	unsigned int cx, cy, p, i;

	unsigned int gy_line_size = (w >> 2); // w / bps 
	unsigned int gu_line_size = (w >> 3); // w / bps
	unsigned int gv_line_size = gu_line_size; // w / bps

	hframe->res_h = hframe->res_w = 16;

	for (cy = 0, i = 0, _gy = gy; cy < h; cy++) {
		for (cx = 0; cx < gy_line_size; cx++) {
			p = *(y + i);
			_gy[cx >> 2] += (p & 0xff) + ((p >> 8)&0xff) + ((p >> 16)&0xff) + ((p >> 24)&0xff);
			i++;
		}
		if ((cy & 0xf) == 0xf) _gy += gmx;
	}
	for (cy = 0, i = 0, _gu = gu; cy < h; cy++) {
		for (cx = 0; cx < gu_line_size; cx++) {
			p = *(u + i);
			_gu[cx >> 1] += (p & 0xff) + ((p >> 8)&0xff) + ((p >> 16)&0xff) + ((p >> 24)&0xff);
			i++;
		}
		if ((cy & 0xf) == 0xf) _gu += gmx;
	}
	for (cy = 0, i = 0, _gv = gv; cy < h; cy++) {
		for (cx = 0; cx < gv_line_size; cx++) {
			p = *(v + i);
			_gv[cx >> 1] += (p & 0xff) + ((p >> 8)&0xff) + ((p >> 16)&0xff) + ((p >> 24)&0xff);
			i++;
		}
		if ((cy & 0xf) == 0xf) _gv += gmx;
	}

	for (i = 0; i < gmx * gmy; i++) {
		gy[i] >>= 8;
		gu[i] >>= 7;
		gv[i] >>= 7;
	}
	hframe->w = gmx;
	hframe->h = gmy;
}
