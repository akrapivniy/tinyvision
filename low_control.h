

int init_controller();

void set_user_control (char dir, char speed);
void set_vision_control (char dir, char speed);
void set_control_prio (char user);
char get_control_prio ();

unsigned int get_motor_power ();

