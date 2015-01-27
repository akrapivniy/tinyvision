#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "vision_core.h"

int abs(int x)
{
    int minus_flag = x>>0x1F;//0x1F = 31
    return ((minus_flag ^ x) - minus_flag);
}

