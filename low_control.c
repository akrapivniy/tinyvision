#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


char *fire_portname = "/dev/ttyATH0";

struct lcontrol {
	int fd;
	char user_prio;
	char user_dir;
	char user_speed;
	char vision_dir;
	char vision_speed;
};

struct lcontrol control;

int set_interface_attribs (int fd, int speed)
{
        struct termios tty;

        memset (&tty, 0, sizeof (struct termios));
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

int init_controller()
{
int err = 0;

 control.fd = open (fire_portname, O_RDWR | O_NOCTTY | O_SYNC);
 if (control.fd < 0)
 {
        printf ("error %d opening %s: %s\n", errno, fire_portname, strerror (errno));
        return -1;
 }

 err = set_interface_attribs (control.fd, B115200);
 return err;
}

int motor (int x,int y)
{
 char buffer[10];

 buffer[0]=0x55;
 buffer[1]=x;
 buffer[2]=y;
 buffer[3]=0xAA;
 write (control.fd, buffer , 4);
 return 0;
}

void set_control (char dir, char speed) 
{
switch (dir) {
	case 'F':motor (speed,speed);break;
	case 'B':motor (-speed,-speed);break;

	case 'L':motor (-speed,speed);break;
	case 'l':motor (speed/2,speed);break;

	case 'R':motor (speed,-speed);break;
	case 'r':motor (speed,speed/2);break;

	case 'S': 
	case   0: 
	default: motor (0,0);
}
}


void set_user_control (char dir, char speed)
{
	control.user_dir = dir;
	control.user_speed = speed;
	if (!control.user_prio) return;

	set_control (dir,speed);
}

void set_vision_control (char dir, char speed)
{
	control.vision_dir = dir;
	control.vision_speed = speed;
	if (control.user_prio) return;

	set_control (dir,speed);
}

void set_control_prio (char user)
{
	control.user_prio = user;
	if (control.user_prio) {
		set_control (control.user_dir,control.user_speed);   
	} else {
		set_control (control.vision_dir,control.vision_speed);   
	} 
}

char get_control_prio ()
{
	return control.user_prio;

}

unsigned int get_motor_power ()
{
	
}

void get_control_state (char *dir, char *speed) 
{
   if (control.user_prio) {
	*dir = control.user_dir; 
	*speed = control.user_speed;   
   } else {
	*dir = control.vision_dir; 
	*speed = control.vision_speed;   
   } 
}


