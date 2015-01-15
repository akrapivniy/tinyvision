/* 
*      control.c
*
*      HTTP Control interface for uvc_stream
*      
*      2007 Lucas van Staden (lvs@softhome.net) 
*
*        Please send ONLY GET requests with the following commands:"
*        "PAN     : ?pan=[-]<pan value> -  Pan camera left or right"
*        "TILT    : ?tilt=[-]<tilt value>; - Tilt camera up or down"
*        "RESET   : ?reset=<value> - Reset 1 - pan, 2 tilt, 3 pan/tilt"
*        "CONTROL : ?control - control screen with buttons to pan and tilt"
*
*	
*      this code is heavily based on the webhttpd.c code from the motion project
*
*      Copyright 2004-2005 by Angel Carpintero  (ack@telefonica.net)
*      This software is distributed under the GNU Public License Version 2
*
*/
#include "control.h"
#include "v4l2uvc.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>          
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "low_control.h"
       

int uvc_reset(struct control_data *cd, int reset)
{
    /*int reset = 3; //0-non reset, 1-reset pan, 2-reset tilt, 3-reset pan&tilt */
    struct v4l2_control control_s;
    control_s.id = V4L2_CID_PANTILT_RESET;
    control_s.value = reset;
    if (ioctl(cd->video_dev, VIDIOC_S_CTRL, &control_s) < 0) {
    	fprintf(stderr, "failed to control motor\n");
	return 0;
    }
    return 1;
}




static int read_client(int client_socket, struct control_data *cd)
{
	char buffer[1024] = {'\0'};
	int length = 1024;


	while (1)
	{
		int nread = 0;

		nread = read (client_socket, buffer, length);

		if (nread <= 0) 
			return -1;
		
		if ( nread != 4 ) continue;
		
		switch (buffer[1]) {
		case 'u':
			set_control_prio (1);
			break;
		case 'm':
			set_control_prio (0);
			break;
		case 'F':
		case 'B':
		case 'L':
		case 'l':
		case 'R':
		case 'r':
		case 'S': 
			set_user_control (buffer[1],buffer[2]);
			break;
		default: 
			set_user_control ('S',0);
			break;		
		}
	}

	return 0;
}




/* #########################################################################
uvcstream_control
######################################################################### */
void control(struct control_data *cd)
{
  struct sockaddr_in addr;
  int sd, on;
  int client_socket_fd;
  struct sigaction act;
  
  /* set signal handlers TO IGNORE */
  memset(&act,0,sizeof(act));
  sigemptyset(&act.sa_mask);
  act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE,&act,NULL);
  sigaction(SIGCHLD,&act,NULL);


  /* open socket for server */
  sd = socket(PF_INET, SOCK_STREAM, 0);
  if ( sd < 0 ) {
    fprintf(stderr, "control socket failed\n");
    exit(1);
  }

  /* ignore "socket already in use" errors */
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
    //exit(1);
    close(sd);
    return;
  }

  /* configure server address to listen to all local IPs */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = cd->control_port;
  addr.sin_addr.s_addr = INADDR_ANY;
  if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
    fprintf(stderr, "control port bind failed\n");
    //exit(1);
    close(sd);
    return;
  } 

  /* start listening on socket */
  if ( listen(sd, 5) == -1 ) {
    fprintf(stderr, "control port listen failed\n");
    close(sd);
    return;
  }

  while ( 1 ) {
    client_socket_fd = accept(sd, 0, 0);
    if (client_socket_fd<=0) continue;
    read_client(client_socket_fd, cd);
    close(client_socket_fd);
  }

  close(sd);
}

void *uvcstream_control(void *arg)
{	
	struct control_data *cd=arg;
	control(cd);
	pthread_exit(NULL);
}
          
