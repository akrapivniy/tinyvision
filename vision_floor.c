#include <stdio.h>
#include <string.h>
#include "vision_core.h"
#include "vision_floor.h"

int vision_init_floor_data (struct vision_floor_data *fd)
{
	fd->w = fd->h = fd->fw = 0;
	fd->floor_prev = fd->floor_data + VISION_MAX_WIDTH;
	fd->floor_current = fd->floor_data;

	memset (fd->floor_speed, 0, VISION_MAX_WIDTH*sizeof(int));
	memset (fd->floor_state, 0, VISION_MAX_WIDTH*sizeof(int));
	memset (fd->floor_next, 0, VISION_MAX_WIDTH*sizeof(int));
	memset (fd->floor_data, 0, VISION_MAX_WIDTH*2*sizeof(int));
	
	return 0;
}


//middle from 8 px
int get_floor_h8_y (unsigned int *y, unsigned int w, unsigned int h, int threshold, int *floor_level)
{
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int floor_size = w >> 3; // every  8x8 pixel
    unsigned int gy_line_size = (w>>2); // w / bps 
    unsigned int *_y = y + y_size - gy_line_size;
    unsigned int floor_avg[VISION_MAX_WIDTH] = {0};
    unsigned int p, p2, i;
    int clean, level, diff, his;

    
	for (i = 0; i < floor_size; i++) {
		p = *(_y+(i<<1));
		p2 = *(_y+(i<<1)+1);
		floor_avg[i]=(p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
		floor_avg[i]+=(p2&0xff) + ((p2>>8)&0xff) + ((p2>>16)&0xff) + ((p2>>24)&0xff);
		floor_avg[i]/=8;
	}

	_y -= gy_line_size;
	for (level = h - 2; _y > y; _y-=gy_line_size, level--) {
		clean = 1;    
        	for (i = 0; i < floor_size; i++) {
			if (floor_level[i]>level) continue;
			clean = 0;
			p = *(_y+(i<<1));
			p2 = *(_y+(i<<1)+1);
			his=(p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
			his+=(p2&0xff) + ((p2>>8)&0xff) + ((p2>>16)&0xff) + ((p2>>24)&0xff);
			his/=8;
			diff = his - floor_avg[i];
			if (abs(diff) > threshold) {
				floor_level[i] = level;
			}
			floor_avg[i]+=diff/4;
		}
		if (clean) break;
	}
	return 0;
}

//middle from 8 px
int get_floor_h8_uv (unsigned int *uv, int w, int h, int threshold, int *floor_level)
{
    unsigned int pixel_size = (w * h);
    unsigned int uv_size = pixel_size>>3;
    unsigned int floor_size = w >> 3; // every  8x8 pixel
    unsigned int guv_line_size = (w>>3); // w / bps
    unsigned int *_uv = uv + uv_size - guv_line_size;
    int floor_avg[VISION_MAX_WIDTH] = {0};
    unsigned int p, i;
    int clean, level, diff, his;

	for (i = 0; i < floor_size; i++) {
		p = *(_uv+i);
		floor_avg[i]=(p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
		floor_avg[i]/=4;
	}

	_uv -= guv_line_size;
	for (level = h - 2; _uv > uv; _uv-=guv_line_size, level--) {
		clean = 1;    
        	for (i = 0; i < floor_size; i++) {
			if (floor_level[i]>level) continue;
			clean = 0;
			p = *(_uv+i);
			his=(p&0xff) + ((p>>8)&0xff) + ((p>>16)&0xff) + ((p>>24)&0xff);
			his/=4;
			diff = his - floor_avg[i];
			if (abs(diff) > threshold) {
				floor_level[i] = level;
			}
			floor_avg[i]+=diff/4;
		}
		if (clean) break;
	}
	return 0;
}

int direct_floor_level_min (struct v_frame *frame, int *floor_level)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int u_size = pixel_size>>3;
    unsigned int *y = frame->pixmap;
    unsigned int *u = y + y_size;
    unsigned int *v = u + u_size;

    get_floor_h8_y (y, w, h, 30, floor_level);
    get_floor_h8_uv (v , w, h, 5, floor_level);
    get_floor_h8_uv (u , w, h, 5, floor_level);
    return 0;
}

int direct_hyst_floor_level_min (struct v_histogram_frame *hf, struct vision_floor_color *color, int *floor_level)
{
    unsigned int hw = hf->w;
    unsigned int hh = hf->h;
    unsigned int h_size = hw*hh;
    unsigned int w = hw*hf->res_w;
    unsigned int h = hh*hf->res_h;
    unsigned int *y = hf->y + h_size-1;
    unsigned int *u = hf->u + h_size-1;
    unsigned int *v = hf->v + h_size-1;
    unsigned int level, i, fl;
    unsigned int ty;
    unsigned int tu;
    unsigned int tv;
    
    ty = (abs (color->max_y - color->min_y) ) / 2;
    tu = (abs (color->max_u - color->min_u) ) / 2;
    tv = (abs (color->max_v - color->min_v) ) / 2;
    
    printf ("ty=%d,tu=%d,tv=%d \n",ty,tu,tv);
    
    
    for (level = hh; level > 0; level--) 
	    for (i = hw; i > 0; i--, y--, u--, v--) {
		fl=(i-1)*2;
		if (floor_level[fl]) continue;
		if (abs (*y-color->avg_y) > ty) {
			floor_level[fl] = floor_level[fl+1] = level*hf->res_h+8;
			continue;
		}
		if (abs (*v - color->avg_v) > tv) {
			floor_level[fl] = floor_level[fl+1] = level*hf->res_h+8;
			continue;
		}
		if (abs (*u - color->avg_u) > tu) {
			floor_level[fl] = floor_level[fl+1] = level*hf->res_h+8;
			continue;
		}
	    }
    
}


int direct_floor_color (struct v_frame *frame, int *floor_level, struct vision_floor_color *color)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int pixel_size = (w * h);
    unsigned int y_size = pixel_size>>2;
    unsigned int u_size = pixel_size>>3;
    unsigned int *y = frame->pixmap;
    unsigned int *u = y + y_size;
    unsigned int *v = u + u_size;
    unsigned int floor_size = w >> 3; // every  8x8 pixel
    unsigned int gy_line_size = (w>>2); // w / bps 
    unsigned int guv_line_size = (w>>3); // w / bps
    unsigned int *_y = y;
    unsigned int *_u = u;
    unsigned int *_v = v;
    unsigned int avg_y = 0, avg_count = 0;
    unsigned int avg_u = 0; 
    unsigned int avg_v = 0; 
    unsigned int i, i2;
    int level;
    union {
	unsigned int i;
	unsigned char b[4];
    } py,py2, pv, pu;    

    for (i = 0, level = 0; i < floor_size; i++) 
	    if (floor_level[i] > level) level = floor_level[i];

    printf ("max level = %3d ", level);
    
    _y = y + gy_line_size * level;
    _u = u + guv_line_size * level;
    _v = v + guv_line_size * level;
    color->min_u = *_y; color->min_v = *_v; color->min_y = *_u;
    color->max_u = *_y; color->max_v = *_v; color->max_y = *_u;
    color->avg_u = 0; color->avg_v = 0; color->avg_y = 0;
    for (;level<h ; level++) {
        	for (i = 0; i < floor_size; i++, _y+=2, _v++, _u++ ) {
			if (floor_level[i]+10>level) continue;
			py.i = *_y;
			py2.i = *_y+1;
			for (i2=0;i2<4;i2++) {
				if (color->max_y < py.b[i2])
					color->max_y=(color->max_y+py.b[i2])/2;
				if (color->min_y > py.b[i2])
					color->min_y=(color->min_y+py.b[i2])/2;
				if (color->max_y < py2.b[i2])
					color->max_y=(color->max_y+py2.b[i2])/2;
				if (color->min_y > py2.b[i2])
					color->min_y=(color->min_y+py2.b[i2])/2;
				avg_y+=py.b[i2]+py2.b[i2];
			}
			pv.i = *_v;
			for (i2=0;i2<4;i2++) {
				if (color->max_v < pv.b[i2])
					color->max_v= (color->max_v+pv.b[i2])/2;
				if (color->min_v > pv.b[i2])
					color->min_v= (color->min_v+pv.b[i2])/2;
				avg_v+=pv.b[i2]*2;
			}
			pu.i = *_u;
			for (i2=0;i2<4;i2++) {
				if (color->max_u < pu.b[i2])
					color->max_u=(color->max_u+pu.b[i2])/2;
				if (color->min_u > pu.b[i2])
					color->min_u=(color->min_u+pu.b[i2])/2;;
				avg_u+=pu.b[i2]*2;
			}
			avg_count+=8;
		}
	}
	if (avg_count) {
		color->avg_y = avg_y / avg_count;
		color->avg_u = avg_u / avg_count;
		color->avg_v = avg_v / avg_count;
	}
	return 0;
}

void exchange_floor_data (struct vision_floor_data *fd)
{
	int *tmp_fd;
	tmp_fd = fd->floor_prev;
	fd->floor_prev = fd->floor_current;
	fd->floor_current = tmp_fd;
}

int *vision_get_floor_level (struct v_frame *frame, struct vision_floor_data *fd, int floor_delta_threshold)
{
    unsigned int w = frame->w;
    unsigned int h = frame->h;
    unsigned int fw = w/8;
    int i;
    int avg_speed_dir = 0;
    int threshold = h - floor_delta_threshold;

    fd->w = w; fd->h = h;fd->fw = fw;
    
    exchange_floor_data (fd);
    memset (fd->floor_current,0,fw*sizeof(int));
    direct_floor_level_min (frame, fd->floor_current);

    for (i = 0; i < fw; i++) {
	    if (fd->floor_current[i] == 0) {
		    fd->floor_current[i] = h+1; 
		    fd->floor_state[i] = 2;
		    continue;
	    }
	    if (( abs (fd->floor_next[i] - fd->floor_current[i] ) > 10) && 
		    (fd->floor_state[i] == 0)) {
		    fd->floor_state[i] = 1; 
		    continue;
	    }
    	    fd->floor_state[i] = 0;
	    fd->floor_next[i] = fd->floor_current[i]*2 - fd->floor_prev[i];
     }

    return fd->floor_state;
}


int vision_get_use_floor (struct vision_floor_data *fd, int wide)
{
    unsigned int w = fd->w;
    unsigned int fw = fd->fw;
    unsigned int h = fd->h;
    unsigned int use_wide = ((fw *  wide) / 100);
    unsigned int start_wide = fw / 2 - use_wide / 2;
    unsigned int max_fl = 0;
    int i;
    
    for (i = start_wide; i < start_wide+use_wide; i++) {
	    if  (fd->floor_state[i]&1) continue;
	    if (fd->floor_state[i] == 2) max_fl = h+1; 
	    if ( (abs (fd->floor_current[i] + fd->floor_prev[i])/2) > max_fl) 
			max_fl = abs (fd->floor_current[i] + fd->floor_prev[i])/2;
	    
    }
    if (!max_fl)  max_fl = h+1;
    return max_fl;
}

int vision_get_way_floor (struct vision_floor_data *fd)
{
    unsigned int w = fd->w;
    unsigned int fw = fd->fw;
    unsigned int h = fd->h;
    int i;
    int way_lr = 0;
    int way_dist = 0;
    
    
    for (i = 0; i < fw/2; i++) {
	    if (!fd->floor_state[i]) way_lr ++;
	    way_dist += abs (fd->floor_current[i] - fd->floor_prev[i])/2;
    }
    for (i = fw/2; i < fw; i++) {
	    if (!fd->floor_state[i]) way_lr --;
	    way_dist -= abs (fd->floor_current[i] - fd->floor_prev[i])/2;
    }
    
    printf ("way_lr = %d; way_dist = %d \n",way_lr,way_dist);
    if ( abs (way_lr) > 2 ) {
	    if ( way_lr > 0 ) return -1; 
	    else return 1;
    } else {
	    if ( way_dist > 0 ) return -1; 
	    else if ( way_dist < 0 ) return 1; 
	    else return 0;
    }
    return 0;
}


