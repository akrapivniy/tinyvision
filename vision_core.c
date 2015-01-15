#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_core.h"

int abs(int x)
{
    int minus_flag = x>>0x1F;//0x1F = 31
    return ((minus_flag ^ x) - minus_flag);
}

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

inline void copy_storage (struct v_histogram_frame *dst, struct v_histogram_frame *src)
{
    memcpy (dst->u, src->u, VISION_HISTOGRAM_SIZE * sizeof (int));
    memcpy (dst->v, src->v, VISION_HISTOGRAM_SIZE * sizeof (int));
    memcpy (dst->y, src->y, VISION_HISTOGRAM_SIZE * sizeof (int));
}


int compare_histogram (struct v_histogram_frame *f1, struct v_histogram_frame *f2, 
			unsigned int *uvdiff, unsigned int *ydiff, 
			unsigned int uv_threshold, unsigned int y_threshold)
{
    unsigned int i, count = 0, uvd = 0, yd = 0;
    unsigned int fsize;
    int *hx1;
    int *hx2;
    unsigned int diff;

    fsize = f1->w * f1->h; //check f2


    hx1 = f1->u; hx2 = f2->u; 
    for (i=0; i< fsize; i++) {
	diff = abs(hx1[i] - hx2[i]);
	uvd += diff;
	if (diff > uv_threshold) count++;
    }

    hx1 = f1->v; hx2 = f2->v; 
    for (i=0; i< fsize; i++) {
	diff = abs(hx1[i] - hx2[i]);
	uvd += diff;
	if (diff > uv_threshold) count++;
    }
    *uvdiff = uvd/ (fsize*2);

    hx1 = f1->y; hx2 = f2->y; 
    for (i=0; i< fsize; i++) {
	diff = abs(hx1[i] - hx2[i]);
	yd += diff;
	if (diff > y_threshold) count++;
    }
    *ydiff = yd/fsize;
    return count;
}

int copy_avg_histogram (struct v_histogram_frame *dst, struct v_histogram_frame *src, 
			int uv_act_threshold, int y_act_threshold, int uv_pass_threshold, int y_pass_threshold)
{
    unsigned int i;
    unsigned int fsize;
    int *hsrc_u=src->u;
    int *hsrc_v=src->v;
    int *hsrc_y=src->y;
    int *hdst_u=dst->u;
    int *hdst_v=dst->v;
    int *hdst_y=dst->y;
    int diff_u, diff_y, diff_v;
    int new_u, new_y, new_v;

    fsize = src->w * src->h; //check dst

    for (i=0; i< fsize; i++) {
	diff_u = abs(hsrc_u[i] - hdst_u[i]);
	diff_v = abs(hsrc_v[i] - hdst_v[i]);
	diff_y = abs(hsrc_y[i] - hdst_y[i]);

	if ((diff_u < uv_act_threshold)&&
	    (diff_v < uv_pass_threshold)&&
	    (diff_y < y_pass_threshold))
		new_u = hdst_u[i] + ((hsrc_u[i] - hdst_u[i])/2);
	else new_u = hdst_u[i];

	if ((diff_u < uv_pass_threshold)&&
	    (diff_v < uv_act_threshold)&&
	    (diff_y < y_pass_threshold))
		new_v = hdst_v[i] + ((hsrc_v[i] - hdst_v[i])/2);
	else new_v = hdst_v[i];

	if ((diff_u < uv_pass_threshold)&&
	    (diff_v < uv_pass_threshold)&&
	    (diff_y < y_act_threshold))
		new_y = hdst_y[i] + ((hsrc_y[i] - hdst_y[i])/2);
	else new_y = hdst_y[i];
	hdst_u[i] = new_u;
	hdst_v[i] = new_v; 
	hdst_y[i] = new_y; 
    }

    return 0;
}

void get_his_frame (struct v_frame *frame, struct v_histogram_frame *hframe)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int u_size = pixel_size>>3;
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
    unsigned int cx,cy,p,i;

    unsigned int gy_line_size = (w>>2); // w / bps 
    unsigned int gu_line_size = (w>>3); // w / bps
    unsigned int gv_line_size = gu_line_size; // w / bps

    hframe->res_h = hframe->res_w = 16;
  
//	printf ("\n");
//	printf ("frame w=%d h=%d gw=%d gh=%d\n",w,h,gmx,gmy);
//	printf ("frame gu_line_size=%d y_size=%d, u_size=%d \n",gu_line_size, y_size, u_size);

    for (cy = 0, i = 0, _gy = gy; cy < h; cy++) {
	for (cx = 0; cx < gy_line_size; cx++) {
	    p = *(y + i);
	    _gy[cx>>2]+= (p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
	    i++;
	}
	if ((cy&0xf) == 0xf) _gy+=gmx;
    }
    for (cy = 0, i = 0, _gu = gu; cy < h; cy++) {
	for (cx = 0; cx < gu_line_size; cx++) {
	    p = *(u + i);
	    _gu[cx>>1]+= (p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
	    i++;
	}
	if ((cy&0xf) == 0xf) _gu+=gmx;
    }
    for (cy = 0, i = 0, _gv = gv; cy < h; cy++) {
	for (cx = 0; cx < gv_line_size; cx++) {
	    p = *(v + i);
	    _gv[cx>>1]+= (p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
	    i++;
	}
	if ((cy&0xf) == 0xf) _gv+=gmx;
    }

    for (i = 0; i < gmx*gmy; i++) {
	gy[i] >>= 8;
	gu[i] >>= 7;
	gv[i] >>= 7;
    }
    hframe->w = gmx;
    hframe->h = gmy;
}
	
int get_target_map (struct v_histogram_frame *hf, struct v_histogram_frame *his_background,int *target_map, unsigned int uv_threshold, unsigned int y_threshold)
{
    unsigned int cx,cy, i;
    unsigned int w = hf->w;
    unsigned int h = hf->h;
    int *fy = hf->y;
    int *fu = hf->u;
    int *fv = hf->v;
    int *by = his_background->y;
    int *bu = his_background->u;
    int *bv = his_background->v;
    unsigned int target = 1;
    unsigned int part;


    memset (target_map,0,w*h*4);
    for (cy = 0; cy < h; cy++ ) {
	for (cx = 0; cx < w; cx++ ) {
		i = cy*w+cx;
		if ((abs(fy[i] -  by[i]) > y_threshold)||
		    (abs(fv[i] -  bv[i]) > uv_threshold)||
		    (abs(fu[i] -  bu[i]) > uv_threshold)) {
		    part = 0;
		    if (cx>0) part=target_map[i-1];
		    if ((!part)&&(cy>0)) part=target_map[i-w];
		    if ((!part)&&(cy>0)&&(cx>0)) part=target_map[i-1-w];
		    if ((!part)&&(cy>0)&&(cx<w)) part=target_map[i+1-w];
		    if  (!part) {
				part = target;
				target++;
				}
		    target_map[i] = part;
		}
	}
    }
    return target - 1;
}

int get_targets (struct v_histogram_frame *hf, struct v_histogram_frame *his_background, int *target_map, struct v_target targets[], unsigned int uv_threshold, unsigned int y_threshold)
{
    unsigned int cx,cy, i;
    unsigned int w = hf->w;
    unsigned int h = hf->h;
    int *fy = hf->y;
    int *fu = hf->u;
    int *fv = hf->v;
    int *by = his_background->y;
    int *bu = his_background->u;
    int *bv = his_background->v;
    unsigned int target = 1;
    unsigned int part;
    struct v_target *_target;


    memset (target_map,0,w*h*4);
    for (cy = 0; cy < h; cy++ ) {
	for (cx = 0; cx < w; cx++ ) {
		i = cy*w+cx;
		if ((abs(fy[i] -  by[i]) > y_threshold)||
		    (abs(fv[i] -  bv[i]) > uv_threshold)||
		    (abs(fu[i] -  bu[i]) > uv_threshold)) {
		    part = 0;
		    if (cx>0) part=target_map[i-1];
		    if ((!part)&&(cy>0)) part=target_map[i-w];
		    if ((!part)&&(cy>0)&&(cx>0)) part=target_map[i-1-w];
		    if ((!part)&&(cy>0)&&(cx<w)) part=target_map[i+1-w];
		    if  (!part) {
				target_map[i] = target;
				_target = targets + (target-1);
				target++;
				_target->ex = cx;
				_target->sx = cx;
				_target->ey = cy;
				_target->sy = cy;
				_target->cx = cx;
				_target->cy = cy;
				_target->size = 1;
				_target->hu =fu[i];
				_target->hv =fv[i];
				_target->hy =fy[i];
				_target->du =abs(fu[i] -  bu[i]);
				_target->dv =abs(fv[i] -  bv[i]);
				_target->dy =abs(fy[i] -  by[i]);

		    } else {
			target_map[i] = part;
			_target = targets + (part-1);
			if (cx>_target->ex) _target->ex = cx;
			if (cx<_target->sx) _target->sx = cx;
			if (cy>_target->ey) _target->ey = cy;
			if (cy<_target->sy) _target->sy = cy;
			if (cx>_target->cx) _target->cx++;
			if (cx<_target->cx) _target->cx--;
			if (cy>_target->cy) _target->cy++;
			if (cy<_target->cy) _target->cy--;
			_target->size++;
			_target->hu+=fu[i];
			_target->hv+=fv[i];
			_target->hy+=fy[i];
			_target->du+=abs(fu[i] -  bu[i]);
			_target->dv+=abs(fv[i] -  bv[i]);
			_target->dy+=abs(fy[i] -  by[i]);
		    }
		}
	}
    }
    return target - 1;
}


int find_target (struct v_target targets[], unsigned int tc, struct v_target_lock *main_target, int *fx, int *fy)
{
    int i;

    if (!main_target->state) return 0;
    for (i = 0; i < tc; i++) {
	if (( abs(targets[i].size - main_target->size) < 3)&&
	    ( abs(targets[i].hy - main_target->hy) < 10)&&
	    ( abs(targets[i].hu - main_target->hu) < 2)&&
	    ( abs(targets[i].hv - main_target->hv) < 2)) {
	    *fx = targets[i].cx;
	    *fy = targets[i].cy;
	    main_target->size = targets[i].size;
	    main_target->hu = targets[i].hu;
	    main_target->hv = targets[i].hv;
	    main_target->hu = targets[i].hu;
	    main_target->track++;
	    main_target->dx =  targets[i].cx - main_target->x;
	    main_target->nx = targets[i].cx + main_target->dx;
	    main_target->x = targets[i].cx;
	    main_target->dy = targets[i].cy - main_target->y;
	    main_target->ny = targets[i].cy + main_target->dy;
	    main_target->y = targets[i].cy;
	    return 1;
	}
    }
    return 0;
}


int find_best_target (struct v_target targets[], unsigned int tc, struct v_target_lock *main_target,  int *fx, int *fy)
{
	int max_size = 0, i;
	struct v_target *_target = targets;

	for (i = 0; i<tc; i++) {
		if (max_size < targets[i].size)
			{
			    max_size = targets[i].size;
			    _target = targets+i;
			}
	}
        *fx = _target->cx;
        *fy = _target->cy;
	main_target->x = _target->cx;
	main_target->y = _target->cy;
	main_target->hy = _target->hy;
	main_target->hv = _target->hv;
	main_target->hu = _target->hu;
	main_target->size = _target->hy;
	main_target->state = 1;
	main_target->track = 0;
        return 1;
}
