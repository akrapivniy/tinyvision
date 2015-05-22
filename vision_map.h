/* 
 * File:   vision_map.h
 * Author: akrapivniy
 *
 * Created on 19 Ноябрь 2014 г., 21:01
 */

#ifndef VISION_MAP_H
#define	VISION_MAP_H


void vision_map_init();
int vision_map_get_angle();
void vision_map_set_near_wall(char wl, char wf, char wr);

#endif