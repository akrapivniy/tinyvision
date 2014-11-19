/* 
 * File:   vision_core.h
 * Author: akrapivniy
 *
 * Created on 19 Ноябрь 2014 г., 21:01
 */

#ifndef VISION_CORE_H
#define	VISION_CORE_H

#define VISION_MAX_WIDTH 640
#define VISION_MAX_HEIGHT 480
#define VISION_HISTOGRAM_PIECE 16
#define VISION_HISTOGRAM_SIZE ((VISION_MAX_WIDTH/VISION_HISTOGRAM_PIECE) * (VISION_MAX_HEIGHT/VISION_HISTOGRAM_PIECE))
#define VISION_HISTOGRAM_NUMS 3

struct v_target {
    int sx,sy;	//start
    int ex,ey;	//end
    int cx,cy;	//center
    int size;	//size
    int hy,hu,hv; //histogram
    int dy,du,dv; //diff histogram from back
};

struct v_histogram_frame {
    int *u;
    int *v;
    int *y;
    int w,h;
};

struct v_frame {
    unsigned int *pixmap;
    int w;
    int h;
};

struct v_target_lock {
    int state;
    int x,y;	  //start
    int size;  //size
    int hy,hu,hv; //histogram
    int dx,dy;
    int nx,ny;
    int track;
};

int abs(int x);
int compare_histogram (struct v_histogram_frame *f1, struct v_histogram_frame *f2, 
			unsigned int *uvdiff, unsigned int *ydiff, 
			unsigned int uv_threshold, unsigned int y_threshold);
void get_his_frame (struct v_frame *frame, struct v_histogram_frame *hframe);
int get_target_map (struct v_histogram_frame *hf, struct v_histogram_frame *his_background,int *target_map, unsigned int uv_threshold, unsigned int y_threshold);
int get_targets (struct v_histogram_frame *hf, struct v_histogram_frame *his_background, int *target_map, struct v_target targets[], unsigned int uv_threshold, unsigned int y_threshold);
int find_target (struct v_target targets[], unsigned int tc, struct v_target_lock *main_target, int *fx, int *fy);
int find_best_target (struct v_target targets[], unsigned int tc, struct v_target_lock *main_target,  int *fx, int *fy);

#endif	/* VISION_CORE_H */
