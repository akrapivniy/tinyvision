/* 
 * File:   vision_hide.h
 * Author: akrapivniy
 *
 * Created on 8 мая 2015 г., 16:42
 */

#ifndef VISION_HIDE_H
#define	VISION_HIDE_H

#include "vision_sframe.h"

int vision_hide(struct v_sframe *frame, int mode);
void vision_hide_init(char rotate, int target_size);

#endif	/* VISION_HIDE_H */

