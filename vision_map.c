#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "vision_map.h"
#include "vision_core.h"

struct color {
	char u, v, y;
};

//map map
//0 - unknow
//1 - space
//2 - wall
//3 - color wall
//2 - type xx 


#define VMAP_UNKNOW	0
#define VMAP_SPACE	1
#define VMAP_WALL	2
#define VMAP_WALL2	3

#define VMAP_GET_DETECT_COUNT(m)	(m>>2)
#define VMAP_SET_DETECT_COUNT(m,v)	((m&~3) | (v<<2))
#define VMAP_INC_DETECT_COUNT(m)	((m&~3) != 0xfc)? m+0x4 : m)

struct wifi_ap_data {
	int id;
};

struct visual_map_cell {
	int id;
	int gx, gy;
	int x, y;

	struct wifi_ap_data wifi_ap[5];
	struct color floor;
	struct color wall;
	char map[50][50];
};

struct special_point_data {
	int id;
	int gx, gy;
	char dir;

	struct color floor;
	struct color wall;

	struct color min;
	struct color mid;
	struct color max;
	struct color max_diff;
	struct wifi_ap_data wifi_ap[7];
};

struct visual_map_data {
	int32_t x, y, angle;
	uint32_t update_count;
	uint32_t wr, wf, wl;
};

struct visual_map_data vmap;
struct visual_map_cell current_map;

void vision_map_print()
{
	int ix, iy;
	int mx, my;

	mx = vmap.x / 10;
	my = vmap.y / 10;

	printf("map x=%d y=%d \n", vmap.x, vmap.y);
	for (iy = 0; iy < 50; iy++) {
		for (ix = 0; ix < 50; ix++) {
			if ((iy == my) && (ix == mx)) {
				printf("I");
				continue;
			}
			switch (current_map.map[ix][iy]&3) {
			case 0: printf("?");
				break;
			case 1: printf(" ");
				break;
			case 2: printf("#");
				break;
			case 3: printf("*");
				break;
			}
		}

		printf("\n");
	}
	printf("angle=%d\n", vmap.angle);
	fflush(stdout);
}

void vision_map_move_up()
{
	int sx, sy, dx, dy;

	for (sy = 0, dy = 25; sy < 25; sy++, dy++)
		for (sx = 0, dx = 0; sx < 50; sx++, dx++) {
			current_map.map[dx][dy] = current_map.map[sx][sy];
			current_map.map[sx][sy] = VMAP_UNKNOW;
		}
	vmap.y += 250;
}

void vision_map_move_down()
{
	int sx, sy, dx, dy;

	for (sy = 25, dy = 0; sy < 50; sy++, dy++)
		for (sx = 0, dx = 0; sx < 50; sx++, dx++) {
			current_map.map[dx][dy] = current_map.map[sx][sy];
			current_map.map[sx][sy] = VMAP_UNKNOW;
		}
	vmap.y -= 250;
}

void vision_map_move_left()
{
	int sx, sy, dx, dy;

	for (sy = 0, dy = 0; sy < 50; sy++, dy++)
		for (sx = 25, dx = 0; sx < 50; sx++, dx++) {
			current_map.map[dx][dy] = current_map.map[sx][sy];
			current_map.map[sx][sy] = VMAP_UNKNOW;
		}
	vmap.x -= 250;
}

void vision_map_move_right()
{
	int sx, sy, dx, dy;

	for (sy = 0, dy = 0; sy < 50; sy++, dy++)
		for (sx = 0, dx = 25; sx < 25; sx++, dx++) {
			current_map.map[dx][dy] = current_map.map[sx][sy];
			current_map.map[sx][sy] = VMAP_UNKNOW;
		}
	vmap.x += 250;
}

void vision_map_set_map(uint32_t x, uint32_t y, uint32_t val)
{
	unsigned char cell;

	x /= 10;
	y /= 10;

	if ((x > 50) || (y > 50)) return;

	cell = current_map.map[x][y];

	if ((cell & 0x03) == val) {
		if ((cell & 0xfc) != 0xfc)
			current_map.map[x][y] += 0x04;
	} else
		current_map.map[x][y] = val;
}

void vision_map_apply_camera()
{
	int32_t x, y, dx, dy;

	vision_map_get_coord_offset(vmap.angle, &dx, &dy);
	x = vmap.x + dx * 10;
	y = vmap.y - dy * 10;
	vision_map_set_map(x, y, vmap.wf);
	vmap.wf = VMAP_UNKNOW;

	vision_map_get_coord_offset(vmap.angle - 45, &dx, &dy);
	x = vmap.x + dx * 10;
	y = vmap.y - dy * 10;
	vision_map_set_map(x, y, vmap.wl);
	vmap.wl = VMAP_UNKNOW;

	vision_map_get_coord_offset(vmap.angle + 45, &dx, &dy);
	x = vmap.x + dx * 10;
	y = vmap.y - dy * 10;
	vision_map_set_map(x, y, vmap.wr);
	vmap.wr = VMAP_UNKNOW;
	vision_map_print();
}

void vision_map_update_angle(int32_t angle)
{
	if ((angle < 0) || (angle > 359)) return;
	vmap.angle = angle;
}

int vision_map_get_angle()
{
	return(vmap.angle);
}

void vision_map_update_coordinate(int32_t angle, int32_t dist)
{
	if ((angle < 0) || (angle > 359)) return;

	vmap.x += (int32_t) ((float) dist * sin_table_degree[angle]);
	vmap.y -= (int32_t) ((float) dist * cos_table_degree[angle]);
	vmap.update_count++;
	vmap.angle = angle;

	if (vmap.x < 20) vision_map_move_right();
	else if (vmap.x > 480) vision_map_move_left();

	if (vmap.y < 20) vision_map_move_up();
	else if (vmap.y > 480) vision_map_move_down();

	vision_map_set_map(vmap.x, vmap.y, VMAP_SPACE);
	vision_map_apply_camera();

}

void vision_map_init()
{
	int ix, iy;

	for (iy = 0; iy < 50; iy++)
		for (ix = 0; ix < 50; ix++)
			current_map.map[ix][iy] = VMAP_UNKNOW;

	vmap.x = 250;
	vmap.y = 250;
	vmap.update_count = 0;
	vmap.angle = 0;

}

void vision_map_get_coord_offset(int32_t angle, int32_t *dx, int32_t *dy)
{
	if ((angle >= 337) && (angle <= 23)) {
		*dx = 0;
		*dy = 1;
	}
	else if ((angle >= 23) && (angle <= 68)) {
		*dx = 1;
		*dy = 1;
	}
	else if ((angle >= 68) && (angle <= 113)) {
		*dx = 1;
		*dy = 0;
	}
	else if ((angle >= 113) && (angle <= 158)) {
		*dx = 1;
		*dy = -1;
	}
	else if ((angle >= 158) && (angle <= 203)) {
		*dx = 0;
		*dy = -1;
	}
	else if ((angle >= 203) && (angle <= 248)) {
		*dx = -1;
		*dy = -1;
	}
	else if ((angle >= 248) && (angle <= 293)) {
		*dx = -1;
		*dy = 0;
	}
	else if ((angle >= 293) && (angle <= 337)) {
		*dx = -1;
		*dy = 1;
	}
	else {
		*dx = 0;
		*dy = 0;
	}

	return;
}

void vision_map_set_near_wall(char wl, char wf, char wr)
{
	if (wl) vmap.wl = VMAP_WALL;
	else vmap.wl = VMAP_SPACE;
	if (wr) vmap.wr = VMAP_WALL;
	else vmap.wr = VMAP_SPACE;
	if (wf) vmap.wf = VMAP_WALL;
	else vmap.wf = VMAP_SPACE;
	if (get_control_direction() == 'S')
		vision_map_apply_camera();
}

