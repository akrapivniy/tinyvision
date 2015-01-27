#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_core.h"
#include "vision_hist.h"


void get_histogram (struct v_frame *frame, struct v_histogram *hframe)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int u_size = pixel_size>>3;
//  unsigned int v_size = u_size;
    unsigned int *y = frame->pixmap;
    unsigned int *u = y + y_size;
    unsigned int *v = u + u_size;
    struct histogram8 *hu = hframe->u;
    struct histogram8 *hv = hframe->v;
    struct histogram8 *hy = hframe->y;
    struct histogram8 *_hy;
    struct histogram8 *_hu, *_hv;
    unsigned int hmx = w >> 4; // every  16x16 pixel
    unsigned int hmy = h >> 4; // every  16x16 pixel
    unsigned int cx,cy,i;
    union int32_u p;

    unsigned int hy_line_size = (w>>2); // w / bps 
    unsigned int hu_line_size = (w>>3); // w / bps
    unsigned int hv_line_size = hu_line_size; // w / bps

    hframe->res_h = hframe->res_w = 16;
  
    for (cy = 0, i = 0, _hy = hy; cy < h; cy++) {
	for (cx = 0; cx < hy_line_size; cx++) {
	    p.u32 = *(y + i);
	    _hy[cx>>2].h[p.u8[0]>>5]++;
	    _hy[cx>>2].h[p.u8[2]>>5]++;
	    i++;
	}
	if ((cy&0xf) == 0xf) _hy+=hmx;
    }
    for (cy = 0, i = 0, _hu = hu; cy < h; cy++) {
	for (cx = 0; cx < hu_line_size; cx++) {
	    p.u32 = *(u + i);
	    _hu[cx>>1].h[p.u8[0]>>5]++;
	    _hu[cx>>1].h[p.u8[1]>>5]++;
	    _hu[cx>>1].h[p.u8[2]>>5]++;
	    _hu[cx>>1].h[p.u8[3]>>5]++;
	    i++;
	}
	if ((cy&0xf) == 0xf) _hu+=hmx;
    }
    for (cy = 0, i = 0, _hv = hv; cy < h; cy++) {
	for (cx = 0; cx < hv_line_size; cx++) {
	    p.u32 = *(v + i);
	    _hv[cx>>1].h[p.u8[0]>>5]++;
	    _hv[cx>>1].h[p.u8[1]>>5]++;
	    _hv[cx>>1].h[p.u8[2]>>5]++;
	    _hv[cx>>1].h[p.u8[3]>>5]++;
	    i++;
	}
	if ((cy&0xf) == 0xf) _hv+=hmx;
    }

    hframe->w = hmx;
    hframe->h = hmy;
}

uint8_t compare_histogram (struct histogram8 *h1, struct histogram8 *h2)
{
 uint8_t diff, max_diff, avg_diff=0;	
 int i;
 
  max_diff = abs (h1->h[0] - h2->h[0]);
  for (i = 1; i < 8; i++) {
	  diff = abs (h1->h[i] - h2->h[i]);
	  avg_diff+=diff;
	  if (max_diff < diff)  max_diff=diff;
 }
	
 return avg_diff/8;
}






