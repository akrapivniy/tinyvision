#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


#include "low_control.h"
#include "vision_core.h"
#include "vision_calib.h"

int vision_calibrate_init(struct v_calibrate_state *c, int *table, int w, int h)
{
	c->point_count = 127;
	c->full_count = 0;
	c->uv_threshold = 8;
	c->y_threshold = 10;
	c->x = 127;
	c->y = 127;
	c->w = w;
	c->h = h;
	c->x_dir = 0;
	c->y_dir = 0;
	c->found_x = c->x;
	c->found_y = c->y;
	c->fire_table = table;
	c->direction = 0;
	c->direction_count = 1;
	c->direction_loop = 2;
	//fire (c->x, c->y, 1);
	c->hx = 0;
	c->hy = 0;
	c->x_max = 254;
	c->x_min = 0;
	c->y_max = 254;
	c->y_min = 0;
	return 0;
}

int vision_calibrate_next_point(struct v_calibrate_state *c)
{
	if (c->x > 127) c->x_dir = -1;
	else c->x_dir = 1;
	if (c->y > 127) c->y_dir = 1;
	else c->y_dir = -1;

	c->x = c->found_x;
	c->y = c->found_y;
	c->x += c->x_dir;
	c->y += c->y_dir;

	c->point_count = 20;
	//fire (c->x, c->y, 1);
	do {
		if (c->hy & 1) {
			if (c->hx <= 0)
				c->hy++;
			else
				c->hx--;
		} else {
			if (c->hx >= (c->w - 1))
				c->hy++;
			else
				c->hx++;
			if (c->hy >= (c->h - 1)) return 1;
		}
	} while (c->fire_table[c->hy * c->w + c->hx]);
	printf("Search new point  (%d:%d)\n", c->hx, c->hy);

	return 0;
}

int vision_calibrate_get_near(struct v_calibrate_state *c, int tx, int ty)
{
	int x, y, i;
	int stx = 0, sty = 0;

	for (x = 0; x < c->w; x++) {
		i = ty * c->w + x;
		if (c->fire_table[i]) {
			stx = c->fire_table[i];
		}
	}
	for (y = 0; y < c->h; y++) {
		i = y * c->w + tx;
		if (c->fire_table[i]) {
			sty = c->fire_table[i];
		}
	}
	if (!stx || !sty) return 0;
	return((stx & 0xff00) | (sty & 0xff));
}

int vision_calibrate_finish(struct v_calibrate_state *c)
{
	int x, y, i;

	//easy way
	printf("00:");
	for (y = 0; y < c->h; y++) {
		for (x = 0; x < c->w; x++) {
			i = y * c->w + x;
			if (!c->fire_table[i])
				c->fire_table[i] = vision_calibrate_get_near(c, x, y);
			printf("%04x-", c->fire_table[i]);
		}
		printf("\n%02d:", y);
	}
	fflush(stdout);
	return 0;
}

int vision_calibrate(struct v_calibrate_state *c, struct v_sframe *his_frame)
{
	struct v_target targets[100];
	unsigned int target_count;
	int target_map[VISION_SFRAME_SIZE];
	int timeout, y, uv;
	int i;

	c->full_count++;
	if (c->full_count > 0xfffff) return -1;

	for (timeout = 0, y = c->y_threshold, uv = c->uv_threshold; timeout < 20; timeout++) {
		//	target_count = get_targets (his_frame, target_map, targets, y, uv);
		if (target_count == 1) break;
		if (target_count == 0) {
			y--;
			if (y < 4) {
				y = c->y_threshold;
				uv--;
				if (uv < 3)
					break;
			}
		}
		if (target_count > 1) {
			uv++;
			if (uv > c->uv_threshold) {
				uv = c->uv_threshold;
				y++;
				if (y > (c->y_threshold * 2))
					break;
			}
		}
	}

	c->point_count--;
	if (c->point_count <= 0)
		vision_calibrate_next_point(c);

	if (target_count && 0) {
		printf("T 0 from %d : sx=%3d sy=%3d ex=%3d ey=%3d cx=%3d cy=%3d size=%3d color=%3d:%3d:%3d diff=%3d:%3d:%3d \n", target_count, targets[i].sx, targets[i].sy,
			targets[i].ex, targets[i].ey,
			targets[i].cx, targets[i].cy,
			targets[i].size,
			targets[i].hu / targets[i].size, targets[i].hv / targets[i].size, targets[i].hy / targets[i].size,
			targets[i].du, targets[i].dv, targets[i].dy);



	}

	if ((target_count == 1)&&(targets[0].dv > 5)) {
		if ((target_count == 1)&&(timeout < 8)&&(targets[0].size < 2)&&(targets[0].dv > 4)) {
			c->point_count++;
		}

		if (targets[0].cx > c->hx) c->x_dir = -1;
		else if (targets[0].cx < c->hx) c->x_dir = 1;
		else if (targets[0].cx == c->hx) c->x_dir = 0;

		if (targets[0].cy > c->hy) c->y_dir = 1;
		else if (targets[0].cy < c->hy) c->y_dir = -1;
		else if (targets[0].cy == c->hy) c->y_dir = 0;

		if (!c->x_dir && !c->y_dir) {
			c->point_hold++;
		}

		if (((timeout == 0)&&(targets[0].size == 1)&&(targets[0].dv > 7)) || (c->point_hold > 10)) {
			c->found_x = c->x;
			c->found_y = c->y;
			c->point_count = 20;
			if (!c->x_dir && !c->y_dir) {
				if (vision_calibrate_next_point(c)) return 1;
				if (targets[0].cx == 0) c->x_min = c->x;
				if (targets[0].cx == c->w - 1) c->x_max = c->x;
				if (targets[0].cy == 0) c->y_max = c->y;
				if (targets[0].cy == c->h - 1) c->y_min = c->y;
			}
			printf("Found point for (%d:%d) in (%d:%d)\n", targets[0].cx, targets[0].cy, c->x, c->y);
			if (!c->fire_table[targets[0].cy * c->w + targets[0].cx]) {
				c->fire_table[targets[0].cy * c->w + targets[0].cx] = (c->x << 8) | c->y;
				vision_save_config();
			}
		}
	} else {
		c->point_hold = 0;
	}

	c->x += c->x_dir;
	c->y += c->y_dir;

	if ((c->x < c->x_min) || (c->y < c->y_min) ||
		(c->x > c->x_max) || (c->y > c->x_max)) {
		if (vision_calibrate_next_point(c)) return 1;
	}
	//fire (c->x, c->y, 1);

	return 0;
}
