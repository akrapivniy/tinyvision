
#ifndef LOW_CONTROL_H
#define	LOW_CONTROL_H

#include "vision_core.h"


int init_controller();

void set_user_control(char dir, char speed);
void set_vision_control(char dir, char speed);
void set_control_prio(char user);
char get_control_prio();
void *uart_control_read(void *arg);
void get_motor_power(int32_t *m1, int32_t *m2);
void reset_motor_power();
void enable_limit(int32_t m1, int32_t m2, int32_t(*cb_limit)());
void disable_limit();
char get_control_direction();
void get_control_state(char *dir, char *speed);

void motor_set_control(char dir, char speed, int dist, int32_t(*cb_limit)());
#endif