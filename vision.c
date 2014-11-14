#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fire.h"


struct v_gframe {
    unsigned int *gu;
    unsigned int *gv;
    unsigned int *gy;
    unsigned int gw,gh;
};

struct v_frame {
    unsigned int *pixmap;
    int w;
    int h;
};

void get_gframe (struct v_frame *frame, struct v_gframe *gframe)
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
    unsigned int *gu = gframe->gu;
    unsigned int *gv = gframe->gv;
    unsigned int *gy = gframe->gy;
    unsigned int *_gy, *_gu, *_gv;
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
    gframe->gw = gmx;
    gframe->gh = gmy;
}

void vision_frame (struct v_frame *frame)
{
    static int flag = 0;
    static int back_gu[1200] = {0};
    static int back_gv[1200] = {0};
    static int back_gy[1200] = {0};
    int gu[1200] = {0};
    int gv[1200] = {0};
    int gy[1200] = {0};

    struct v_gframe gframe;
    unsigned int cx,cy, i;
    unsigned int fx,fy,ff;

    if (flag++>100) {
        gframe.gu = gu;
	gframe.gv = gv;
	gframe.gy = gy;
	get_gframe (frame, &gframe);
    } else {
        gframe.gu = back_gu;
	gframe.gv = back_gv;
	gframe.gy = back_gy;
	get_gframe (frame, &gframe);
	return;
    }

    printf ("\n");    fflush (stdout);
    fx = fy = ff = 0;
    for (cy = 0; cy < gframe.gh; cy++ ) {
	for (cx = 0; cx < gframe.gw; cx++ ) {
		i = cy*gframe.gw+cx;
		printf ("%02d:%02d:%02d-", back_gy[i] -  gy[i] ,back_gu[i] - gu[i], back_gv[i] - gv[i]);
		if ( ((back_gy[i] -  gy[i]) > 20) || ( (back_gy[i] -  gy[i]) < -20) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
//			printf ("Y%02d:%02d:%02d - ",fx,fy, (back_gy[i] -  gy[i]));
			}
		if ( ((back_gu[i] -  gu[i]) > 5) || ( (back_gu[i] -  gu[i]) < -5) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
//			printf ("U%02d:%02d:%02d - ",fx,fy, (back_gu[i] -  gu[i]));
			}
		if ( ((back_gv[i] -  gv[i]) > 5) || ( (back_gv[i] -  gv[i]) < -5) ) {
			fx = 60+(120/gframe.gw)*cx+10;
			fy = 145-(75/gframe.gh)*cy-5;
			ff = 1;
//			printf ("V%02d:%02d:%02d - ",fx,fy, (back_gv[i] -  gv[i]));
		}
	}
//    fire (fx, fy, ff);
     printf ("\n");
    }
    fflush (stdout);
}
