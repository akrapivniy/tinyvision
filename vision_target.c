#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_core.h"
#include "vision_sframe.h"
#include "vision_target.h"

int get_target_map(struct v_sframe *hf, struct v_sframe *his_background, int *target_map, unsigned int uv_threshold, unsigned int y_threshold)
{
	unsigned int cx, cy, i;
	unsigned int w = hf->w;
	unsigned int h = hf->h;
	int *fy = hf->y;
	int *fu = hf->u;
	int *fv = hf->v;
	int *by = his_background->y;
	int *bu = his_background->u;
	int *bv = his_background->v;
	unsigned int target = 1;
	unsigned int part;


	memset(target_map, 0, w * h * 4);
	for (cy = 0; cy < h; cy++) {
		for (cx = 0; cx < w; cx++) {
			i = cy * w + cx;
			if ((abs(fy[i] - by[i]) > y_threshold) ||
				(abs(fv[i] - bv[i]) > uv_threshold) ||
				(abs(fu[i] - bu[i]) > uv_threshold)) {
				part = 0;
				if (cx > 0) part = target_map[i - 1];
				if ((!part)&&(cy > 0)) part = target_map[i - w];
				if ((!part)&&(cy > 0)&&(cx > 0)) part = target_map[i - 1 - w];
				if ((!part)&&(cy > 0)&&(cx < w)) part = target_map[i + 1 - w];
				if (!part) {
					part = target;
					target++;
				}
				target_map[i] = part;
			}
		}
	}
	return target - 1;
}

int get_targets(struct v_sframe *hf, struct v_sframe *his_background, int *target_map, struct v_target targets[], unsigned int uv_threshold, unsigned int y_threshold)
{
	unsigned int cx, cy, i;
	unsigned int w = hf->w;
	unsigned int h = hf->h;
	int *fy = hf->y;
	int *fu = hf->u;
	int *fv = hf->v;
	int *by = his_background->y;
	int *bu = his_background->u;
	int *bv = his_background->v;
	unsigned int target = 1;
	unsigned int part;
	struct v_target *_target;


	memset(target_map, 0, w * h * 4);
	for (cy = 0; cy < h; cy++) {
		for (cx = 0; cx < w; cx++) {
			i = cy * w + cx;
			if ((abs(fy[i] - by[i]) > y_threshold) ||
				(abs(fv[i] - bv[i]) > uv_threshold) ||
				(abs(fu[i] - bu[i]) > uv_threshold)) {
				part = 0;
				if (cx > 0) part = target_map[i - 1];
				if ((!part)&&(cy > 0)) part = target_map[i - w];
				if ((!part)&&(cy > 0)&&(cx > 0)) part = target_map[i - 1 - w];
				if ((!part)&&(cy > 0)&&(cx < w)) part = target_map[i + 1 - w];
				if (!part) {
					target_map[i] = target;
					_target = targets + (target - 1);
					target++;
					_target->ex = cx;
					_target->sx = cx;
					_target->ey = cy;
					_target->sy = cy;
					_target->cx = cx;
					_target->cy = cy;
					_target->size = 1;
					_target->hu = fu[i];
					_target->hv = fv[i];
					_target->hy = fy[i];
					_target->du = abs(fu[i] - bu[i]);
					_target->dv = abs(fv[i] - bv[i]);
					_target->dy = abs(fy[i] - by[i]);

				} else {
					target_map[i] = part;
					_target = targets + (part - 1);
					if (cx > _target->ex) _target->ex = cx;
					if (cx < _target->sx) _target->sx = cx;
					if (cy > _target->ey) _target->ey = cy;
					if (cy < _target->sy) _target->sy = cy;
					if (cx > _target->cx) _target->cx++;
					if (cx < _target->cx) _target->cx--;
					if (cy > _target->cy) _target->cy++;
					if (cy < _target->cy) _target->cy--;
					_target->size++;
					_target->hu += fu[i];
					_target->hv += fv[i];
					_target->hy += fy[i];
					_target->du += abs(fu[i] - bu[i]);
					_target->dv += abs(fv[i] - bv[i]);
					_target->dy += abs(fy[i] - by[i]);
				}
			}
		}
	}
	return target - 1;
}

int find_target(struct v_target targets[], unsigned int tc, struct v_target_lock *main_target, int *fx, int *fy)
{
	int i;

	if (!main_target->state) return 0;
	for (i = 0; i < tc; i++) {
		if ((abs(targets[i].size - main_target->size) < 3)&&
			(abs(targets[i].hy - main_target->hy) < 10)&&
			(abs(targets[i].hu - main_target->hu) < 2)&&
			(abs(targets[i].hv - main_target->hv) < 2)) {
			*fx = targets[i].cx;
			*fy = targets[i].cy;
			main_target->size = targets[i].size;
			main_target->hu = targets[i].hu;
			main_target->hv = targets[i].hv;
			main_target->hu = targets[i].hu;
			main_target->track++;
			main_target->dx = targets[i].cx - main_target->x;
			main_target->nx = targets[i].cx + main_target->dx;
			main_target->x = targets[i].cx;
			main_target->dy = targets[i].cy - main_target->y;
			main_target->ny = targets[i].cy + main_target->dy;
			main_target->y = targets[i].cy;
			return 1;
		}
	}
	return 0;
}

int find_best_target(struct v_target targets[], unsigned int tc, struct v_target_lock *main_target, int *fx, int *fy)
{
	int max_size = 0, i;
	struct v_target *_target = targets;

	for (i = 0; i < tc; i++) {
		if (max_size < targets[i].size) {
			max_size = targets[i].size;
			_target = targets + i;
		}
	}
	*fx = _target->cx;
	*fy = _target->cy;
	main_target->x = _target->cx;
	main_target->y = _target->cy;
	main_target->hy = _target->hy;
	main_target->hv = _target->hv;
	main_target->hu = _target->hu;
	main_target->size = _target->hy;
	main_target->state = 1;
	main_target->track = 0;
	return 1;
}

int vision_check_targets(struct v_sframe *sframe, struct v_sframe *back_sframe)
{

	struct v_target targets[100];
	unsigned int target_count;
	int target_map[VISION_SFRAME_SIZE];
	int i;
	int max_size = 0;

	target_count = get_targets(sframe, back_sframe, target_map, targets, 5, 10);

	for (i = 0; i < target_count; i++)
		if (max_size < targets[i].size)
			max_size = targets[i].size;

	if ((target_count > 2) || (max_size > 5))
		return max_size;
	else return 0;
}