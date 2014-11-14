#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fire.h"
#include "vision.h"


#define VISION_MAX_WIDTH 640
#define VISION_MAX_HEIGHT 480
#define VISION_HISTOGRAM_PIECE 16
#define VISION_HISTOGRAM_SIZE ((VISION_MAX_WIDTH/VISION_HISTOGRAM_PIECE) * (VISION_MAX_HEIGHT/VISION_HISTOGRAM_PIECE))
#define VISION_HISTOGRAM_NUMS 3

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
};

struct v_target {
    int sx,sy;	//start
    int ex,ey;	//end
    int cx,cy;	//center
    int size;	//size
    int hy,hu,hv; //histogram
};

struct v_main_target {
    int state;
    int x,y;	  //start
    int size;  //size
    int hy,hu,hv; //histogram
    int dx,dy;
    int nx,ny;
    int track;
};


struct v_histogram_storage *histogram[VISION_HISTOGRAM_NUMS];

struct v_histogram_frame his_background;
struct v_histogram_frame his_dyn_background;
struct v_histogram_frame his_frame;
struct v_state vision_state;
struct v_main_target main_target;



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
    memset (&main_target, 0, sizeof (struct v_main_target));
    return 0;
}


void get_his_frame (struct v_frame *frame, struct v_histogram_frame *hframe)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int u_size = pixel_size>>3;
    unsigned int v_size = u_size;
    unsigned int *y = frame->pixmap;
    unsigned int *u = y + y_size;
    unsigned int *v = u + u_size;
    int *gu = hframe->u;
    int *gv = hframe->v;
    int *gy = hframe->y;
    int *_gy, *_gu, *_gv;
    unsigned int gmx = w >> 4; // every  16x16 pixel
    unsigned int gmy = h >> 4; // every  16x16 pixel
    unsigned int cx,cy,p,i;

    unsigned int gy_line_size = (w>>2); // w / bps 
    unsigned int gu_line_size = (w>>3); // w / bps
    unsigned int gv_line_size = gu_line_size; // w / bps


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

int abs(int x)
{
    int minus_flag = x>>0x1F;//0x1F = 31
    return ((minus_flag ^ x) - minus_flag);
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


int get_target_map (struct v_histogram_frame *hf, int *target_map, unsigned int uv_threshold, unsigned int y_threshold)
{
    unsigned int cx,cy, i , _i;
    unsigned int w = hf->w;
    unsigned int h = hf->h;
    int *fy = hf->y;
    int *fu = hf->u;
    int *fv = hf->v;
    int *by = his_background.y;
    int *bu = his_background.u;
    int *bv = his_background.v;
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



int get_targets (struct v_histogram_frame *hf, int *target_map, struct v_target targets[], unsigned int uv_threshold, unsigned int y_threshold)
{
    unsigned int cx,cy, i , _i;
    unsigned int w = hf->w;
    unsigned int h = hf->h;
    int *fy = hf->y;
    int *fu = hf->u;
    int *fv = hf->v;
    int *by = his_background.y;
    int *bu = his_background.u;
    int *bv = his_background.v;
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
		    }
		}
	}
    }
    return target - 1;
}


int find_main_target (struct v_target targets[], unsigned int tc, unsigned int *fx, unsigned int *fy)
{
    int i;

    if (!main_target.state) return 0;
    for (i = 0; i < tc; i++) {
	if (( abs(targets[i].size - main_target.size) < 3)&&
	    ( abs(targets[i].hy - main_target.hy) < 10)&&
	    ( abs(targets[i].hu - main_target.hu) < 2)&&
	    ( abs(targets[i].hv - main_target.hv) < 2)) {
	    *fx = targets[i].cx;
	    *fy = targets[i].cy;
	    main_target.size = targets[i].size;
	    main_target.hu = targets[i].hu;
	    main_target.hv = targets[i].hv;
	    main_target.hu = targets[i].hu;
	    main_target.track++;
	    main_target.dx =  targets[i].cx - main_target.x;
	    main_target.nx = targets[i].cx + main_target.dx;
	    main_target.x = targets[i].cx;
	    main_target.dy = targets[i].cy - main_target.y;
	    main_target.ny = targets[i].cy + main_target.dy;
	    main_target.y = targets[i].cy;
	    return 1;
	}
    }
    return 0;
}


int find_best_target (struct v_target targets[], unsigned int tc, unsigned int *fx, unsigned int *fy)
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
	main_target.x = _target->cx;
	main_target.y = _target->cy;
	main_target.hy = _target->hy;
	main_target.hv = _target->hv;
	main_target.hu = _target->hu;
	main_target.size = _target->hy;
	main_target.state = 1;
	main_target.track = 0;
        return 1;
}

static int cal_x=127;
static int cal_y=127;


void vision_frame (struct v_frame *frame)
{
    unsigned int cx,cy,i;
    unsigned int fx,fy, ff;
    struct v_histogram_storage  his_diff;
    struct v_target targets[100];
    unsigned int target_count;
    int target_map[VISION_HISTOGRAM_SIZE];


    if (vision_state.tune_background) {
	vision_tunebackground (frame);
	printf ("Tuning background - %u \n",vision_state.tune_background); fflush(stdout);
	return;
    }
    get_his_frame (frame, &his_frame);
//  target_count = get_target_map (&his_frame, target_map, 5, 10);
    target_count = get_targets (&his_frame, target_map, targets, 5, 10);
    fire (cal_x, cal_y, 1);
    if (target_count == 0) {
	    cal_x++;
	    cal_y--;
	    if ((cal_x<0) || (cal_x>254)) cal_x = 127;
	    if ((cal_y<0) || (cal_y>254)) cal_y = 127;
    }

    if (target_count == 0) return;

    ff = find_main_target (targets, target_count, &fx, &fy);

    if (!ff) {
		ff = find_best_target (targets, target_count, &fx, &fy);
    }

    if (fx>5) cal_x--;
    if (fx<5) cal_x++;
    if (fy>5) cal_y++;
    if (fy<5) cal_y--;
    if ((cal_x<0) || (cal_x>254)) cal_x = 127;
    if ((cal_y<0) || (cal_y>254)) cal_y = 127;
    fire (cal_x, cal_y, 1);
    printf ("\n Target fx = %d; fy = %d; \n", fx, fy);    fflush (stdout);

//    fx = 60+(120/his_frame.w)*fx+10;
//    fy = 145-(75/his_frame.h)*fy-5;

/*  printf ("\n count = %d; \n", target_count);    fflush (stdout);
    for (cy = 0; cy < his_frame.h; cy++ ) {
	for (cx = 0; cx < his_frame.w; cx++ ) {
		i = cy*his_frame.w+cx;
		printf ("%02d:%02d:%02d-", 	abs(his_frame.y[i] -  his_background.y[i]),
						abs(his_frame.v[i] -  his_background.v[i]),
						abs(his_frame.u[i] -  his_background.u[i]));
	}
     printf ("\n");
*/
    for (i=0;i<target_count;i++) {
	    printf ("Target %d: sx=%3d sy=%3d ex=%3d ey=%3d cx=%3d cy=%3d size=%3d color=%3d:%3d:%3d \n",i, targets[i].sx,targets[i].sy, 
									targets[i].ex,targets[i].ey,
									targets[i].cx,targets[i].cy,
									targets[i].size,
									targets[i].hu/targets[i].size,targets[i].hv/targets[i].size,targets[i].hy/targets[i].size);
    }

/*    printf ("\n count = %d; \n", target_count);    fflush (stdout);
    for (cy = 0; cy < his_frame.h; cy++ ) {
	for (cx = 0; cx < his_frame.w; cx++ ) {
		i = cy*his_frame.w+cx;
		printf ("%01d-",target_map[i]);
	}
     printf ("\n");
    }*/
    fflush (stdout);
}


/*
fires ()
{
		if ( ((back_gy[i] -  gy[i]) > 20) || ( (back_gy[i] -  gy[i]) < -20) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
			printf ("Y%02d:%02d:%02d - ",fx,fy, (back_gy[i] -  gy[i]));
			}
		if ( ((back_gu[i] -  gu[i]) > 5) || ( (back_gu[i] -  gu[i]) < -5) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
			printf ("U%02d:%02d:%02d - ",fx,fy, (back_gu[i] -  gu[i]));
			}
		if ( ((back_gv[i] -  gv[i]) > 5) || ( (back_gv[i] -  gv[i]) < -5) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
			printf ("V%02d:%02d:%02d - ",fx,fy, (back_gv[i] -  gv[i]));
	}
//    fire (fx, fy, ff);

}
*/