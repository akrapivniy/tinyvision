#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "vision_core.h"
#include "low_control.h"


char *fire_portname = "/dev/ttyATH0";

struct lcontrol {
	int fd;
	char user_prio;
	char user_dir;
	char user_speed;
	char vision_dir;
	char vision_speed;
	int32_t m1;
	int32_t m2;

	int enable_limit;
	int32_t limit_m1;
	int32_t limit_m2;
	int32_t(*cb_limit)(void *);
	void *cb_limit_obj;

	uint32_t dir;
	uint32_t speed;
	int32_t power_angle;
	int32_t power_dist;
	int32_t angle;
};

struct lcontrol lc;


int32_t lc_power_per_sm[] = {0, 0, 0, 4100, 3900, 3600,
	3300, 2700, 1850, 1350, 900};
int32_t lc_power_per_degree[] = {0, 0, 0, 0, 0, 1700,
	1100, 1000, 900, 800, 700};

int set_interface_attribs(int fd, int speed)
{
	struct termios tty;

	memset(&tty, 0, sizeof(struct termios));
	if (tcgetattr(fd, &tty) != 0) {
		printf("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
	tty.c_iflag &= ~IGNBRK; // disable break processing
	tty.c_lflag = 0; // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0; // no remapping, no delays
	tty.c_cc[VMIN] = 0; // read doesn't block
	tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

int init_controller()
{
	int err = 0;

	memset(&lc, 0, sizeof(struct lcontrol));

	lc.fd = open(fire_portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (lc.fd < 0) {
		printf("error %d opening %s: %s\n", errno, fire_portname, strerror(errno));
		return -1;
	}

	err = set_interface_attribs(lc.fd, B115200);
	lc.vision_dir = lc.user_dir = lc.dir = 'S';
	disable_limit();
	motor(0, 0);

	return err;
}

int motor(int x, int y)
{
	char buffer[10];

	buffer[0] = 0x55;
	buffer[1] = x;
	buffer[2] = y;
	buffer[3] = 0xAA;
	if (lc.fd > 0)
		write(lc.fd, buffer, 4);
	else {
		printf("[Motor %d:%d ]", x, y);
		fflush(stdout);
	}
	return 0;
}

void set_user_control(char dir, char speed)
{
	lc.user_dir = dir;
	lc.user_speed = speed;
	//	if (!lc.user_prio) return;

	motor_set_control(dir, speed, 0, NULL);
}

void set_vision_control(char dir, char speed)
{
	lc.vision_dir = dir;
	lc.vision_speed = speed;
	if (lc.user_prio) return;

	motor_set_control(dir, speed, 0, NULL);
}

void set_control_prio(char user)
{
	lc.user_prio = user;
	if (lc.user_prio) {
		motor_set_control(lc.user_dir, lc.user_speed, 0, NULL);
	} else {
		motor_set_control(lc.vision_dir, lc.vision_speed, 0, NULL);
	}
}

char get_control_prio()
{
	return lc.user_prio;

}

void get_motor_power(int32_t *m1, int32_t *m2)
{
	*m1 = lc.m1;
	*m2 = lc.m2;
}

void reset_motor_power()
{
	lc.m1 = 0;
	lc.m2 = 0;
}

void get_control_state(char *dir, char *speed)
{
	*dir = lc.dir;
	*speed = lc.speed;
}

char get_control_direction()
{
	return(lc.dir);
}

void enable_limit(int32_t m1, int32_t m2, int32_t(*cb_limit)())
{
	lc.limit_m1 = m1;
	lc.limit_m2 = m2;
	lc.cb_limit = cb_limit;
	lc.enable_limit = 1;
	//	printf ("limit=%d:%d\n",m1,m2);fflush(stdout);
}

void disable_limit()
{
	lc.enable_limit = 0;
}

void check_limit()
{
	if (lc.limit_m1) {
		if (((lc.limit_m1 > 0) && (lc.m1 > lc.limit_m1)) ||
			((lc.limit_m1 < 0) && (lc.m1 < lc.limit_m1))) {
			motor(0, 0);
			lc.dir = 'S';
			lc.enable_limit = 0;
			if (lc.cb_limit)
				lc.cb_limit(lc.cb_limit_obj);
			return;
		}
	}

	if (lc.limit_m2) {
		if (((lc.limit_m2 > 0) && (lc.m2 > lc.limit_m2)) ||
			((lc.limit_m2 < 0) && (lc.m2 < lc.limit_m2))) {
			motor(0, 0);
			lc.dir = 'S';
			lc.enable_limit = 0;
			if (lc.cb_limit)
				lc.cb_limit(lc.cb_limit_obj);
			return;
		}
	}
	return;
}

void update_coordinates(int32_t m1, int32_t m2)
{
	int32_t angle;
	int32_t dist;

	if (!lc.power_angle && !lc.power_dist && !m1 && !m2)
		return;

	angle = (m1 - m2) / (lc.power_angle * 2);
	dist = (m1 + m2) / (lc.power_dist * 2);

	if (abs(angle) > 5) {
		if (angle > 0)
			lc.angle = (lc.angle + angle) % 360;
		else
			lc.angle = (lc.angle + 360 + angle) % 360;
		//	printf ("[angle=%d]",lc.angle);
		vision_map_update_angle(lc.angle);
	}

	if (abs(dist) > 3) {
		vision_map_update_coordinate(lc.angle, dist);
	}
}

struct motor_power_message {
	uint8_t head;
	int32_t m1;
	int32_t m2;
	uint8_t tail;
} __attribute__((packed));

swap_byte32(uint32_t val)
{
	return(((val >> 24)&0xff) | // move byte 3 to byte 0
		((val << 8)&0xff0000) | // move byte 1 to byte 2
		((val >> 8)&0xff00) | // move byte 2 to byte 1
		((val << 24)&0xff000000)); // byte 0 to byte 3
}

void uart_control_read_loop()
{

	fd_set readfs; /* file descriptor set */
	struct timeval tm;
	int ret, i;
	uint8_t b;

	union {
		uint8_t b[10];
		struct motor_power_message m;
	} msg;
	uint32_t count = 0;

	if (!lc.fd) return;
	tcflush(lc.fd, TCIOFLUSH);
	while (1) {
		FD_ZERO(&readfs);
		FD_SET(lc.fd, &readfs);
		/* set timeout value within input loop */
		tm.tv_usec = 0; /* milliseconds */
		tm.tv_sec = 10; /* seconds */
		ret = select(lc.fd + 1, &readfs, NULL, NULL, &tm);
		if (ret == 0)
			continue;
		while (read(lc.fd, &b, 1)) {
			if ((count == 0) && (b != 0x33)) continue;
			msg.b[count] = b;
			count++;
			if (count == 10) {
				count = 0;
				if (b == 0x99) {
					msg.m.m1 = swap_byte32(msg.m.m1);
					msg.m.m2 = swap_byte32(msg.m.m2);
					//					printf ("m1=%d m2=%d\n",msg.m.m1, msg.m.m2);fflush(stdout);
					lc.m1 += msg.m.m1;
					lc.m2 += msg.m.m2;
					if (lc.enable_limit)
						check_limit();
					update_coordinates(msg.m.m1, msg.m.m2);
				}
			}
		}


	}

}

void *uart_control_read(void *arg)
{
	uart_control_read_loop();
	printf("Exit from uart read \n");
	pthread_exit(NULL);
}

void motor_set_control(char dir, char speed, int dist, int32_t(*cb_limit)())
{
	int32_t dk;
	int32_t ak;

	printf("motor %c %d \n", dir, speed);

	lc.speed = speed;
	lc.dir = dir;
	if ((dir == 'S') || (dir == 0)) {
		disable_limit();
		motor(0, 0);
	}

	dk = lc_power_per_sm[speed / 10];
	ak = lc_power_per_degree[speed / 10];

	if ((dk == 0) || (ak == 0)) {
		lc.dir = 'S';
		disable_limit();
		motor(0, 0);
		return;
	}

	lc.m1 = 0;
	lc.m2 = 0;
	lc.power_angle = ak;
	lc.power_dist = dk;

	switch (dir) {
	case 'F':motor(speed, speed);
		enable_limit(dist*dk, dist*dk, cb_limit);
		break;
	case 'B':motor(-speed, -speed);
		enable_limit(-dist*dk, -dist*dk, cb_limit);
		break;

	case 'L':motor(-speed, speed);
		enable_limit(-dist*ak, dist*ak, cb_limit);
		break;
	case 'l':motor(speed / 2, speed);
		enable_limit(0, dist*dk, cb_limit);
		break;

	case 'R':motor(speed, -speed);
		enable_limit(dist*ak, -dist*ak, cb_limit);
		break;
	case 'r':motor(speed, speed / 2);
		enable_limit(dist*dk, 0, cb_limit);
		break;

	case 'S':
	case 0:
	default:
		lc.dir = 'S';
		disable_limit();
		motor(0, 0);
		break;
	}
}
