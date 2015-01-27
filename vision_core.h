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

typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef signed int 	int32_t;
typedef unsigned int 	uint32_t;

union int32_u {
	uint32_t u32;
	uint8_t  u8[4];
};  

struct v_frame {
    unsigned int *pixmap;
    int w;
    int h;
};

int abs(int x);

#endif	/* VISION_CORE_H */

