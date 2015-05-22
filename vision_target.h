
#ifndef VISION_TARGET_H
#define	VISION_TARGET_H

struct v_target {
	int sx, sy; //start
	int ex, ey; //end
	int cx, cy; //center
	int size; //size
	int hy, hu, hv; //histogram
	int dy, du, dv; //diff histogram from back
};

struct v_target_lock {
	int state;
	int x, y; //start
	int size; //size
	int hy, hu, hv; //histogram
	int dx, dy;
	int nx, ny;
	int track;
};

int vision_check_targets(struct v_sframe *sframe, struct v_sframe *back_sframe);

#endif	/* VISION_TARGET_H */

