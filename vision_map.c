#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_map.h"



struct color {
	char u,v,y;
};

//map map
//0 - unknow
//1 - space
//2 - wall
//3 - color wall
//2 - type xx 

struct wifi_ap_data {
	int id;	
};

struct map_cell {
	int id;
	int gx,gy;
	int x,y;
	
	struct wifi_ap_data wifi_ap[5];
	struct color floor;
	struct color wall;
	char map[10][10];
};

struct special_point_data {
	int id;
	int gx,gy;
	char dir;

	struct color floor;
	struct color wall;

	struct color min;
	struct color mid;
	struct color max;
	struct color max_diff;
	struct wifi_ap_data wifi_ap[7];
};





int fill_special_point ()
{
	
	
}





