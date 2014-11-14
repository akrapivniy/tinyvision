#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


char *fire_portname = "/dev/ttyATH0";
int fire_fd;


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



int init_fire ()
{
int err = 0;

 fire_fd = open (fire_portname, O_RDWR | O_NOCTTY | O_SYNC);
 if (fire_fd < 0)
 {
        printf ("error %d opening %s: %s\n", errno, fire_portname, strerror (errno));
        return -1;
 }

 err = set_interface_attribs (fire_fd, B115200);
 return err;
}


int fire (int x,int y,int f)
{
 char buffer[10];

 buffer[0]=0x55;
 buffer[1]=x;
 buffer[2]=y;
 buffer[3]=f;
 buffer[4]=0xAA;
 write (fire_fd, buffer , 5);
 return 0;
}
