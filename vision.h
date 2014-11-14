#ifndef __VISION_C__
#define __VISION_C__
struct v_histogram_frame {
    int *u;
    int *v;
    int *y;
    int w,h;
};

struct v_frame {
    unsigned int *pixmap;
    int w;
    int h;
};




void vision_frame (struct v_frame *frame);
#endif