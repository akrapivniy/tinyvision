#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "vision_core.h"

float sin_table_degree[360];
float cos_table_degree[360];

int abs(int x)
{
	int minus_flag = x >> 0x1F; //0x1F = 31
	return((minus_flag ^ x) - minus_flag);
}

void vision_core_init()
{
	int i;
	double rad;

	for (i = 0; i < 360; i++) {
		rad = i * (M_PI / 180.0);
		sin_table_degree[i] = (float) sin(rad);
		cos_table_degree[i] = (float) cos(rad);
	}

}

