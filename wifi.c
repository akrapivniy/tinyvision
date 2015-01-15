#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
 
int vision_radio_distance(int signal_db, int freq_khz) {
    double exp = (87.55 - (20 * log10((double)freq_khz)) + fabs((double)signal_db)) / 20.0;
    double dist = pow(10.0, exp);
    return (int)dist;
}

//struct to hold collected information
struct signalInfo {
    char mac[18];
    char ssid[33];
    int bitrate;
    int level;
};

int getSignalInfo(struct signalInfo *sigInfo, char *iwname){
    struct iwreq req;
    //SIOCGIFHWADDR for mac addr
    struct ifreq req2;
    struct iw_statistics *stats;
    int i;

    
    strcpy(req.ifr_name, iwname);
 
    //have to use a socket for ioctl
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 
    //make room for the iw_statistics object
    req.u.data.pointer = (struct iw_statistics *)malloc(sizeof(struct iw_statistics));
    req.u.data.length = sizeof(struct iw_statistics);
 
    //this will gather the signal strength
    if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1){
        //die with error, invalid interface
        fprintf(stderr, "Invalid interface.\n");
        return(-1);
    }
    else if(((struct iw_statistics *)req.u.data.pointer)->qual.updated & IW_QUAL_DBM){
        //signal is measured in dBm and is valid for us to use
        sigInfo->level=((struct iw_statistics *)req.u.data.pointer)->qual.level - 256;
    }
 
    //SIOCGIWESSID for ssid
    char buffer[32];
    memset(buffer, 0, 32);
    req.u.essid.pointer = buffer;
    req.u.essid.length = 32;
    //this will gather the SSID of the connected network
    if(ioctl(sockfd, SIOCGIWESSID, &req) == -1){
        //die with error, invalid interface
        return(-1);
    }
    else{
        memcpy(&sigInfo->ssid, req.u.essid.pointer, req.u.essid.length);
        memset(&sigInfo->ssid[req.u.essid.length],0,1);
    }
 
    //SIOCGIWRATE for bits/sec (convert to mbit)
    int bitrate=-1;
    //this will get the bitrate of the link
    if(ioctl(sockfd, SIOCGIWRATE, &req) == -1){
        fprintf(stderr, "bitratefail");
        return(-1);
    }else{
        memcpy(&bitrate, &req.u.bitrate, sizeof(int));
        sigInfo->bitrate=bitrate/1000000;
    }
 
 
    strcpy(req2.ifr_name, iwname);
    //this will get the mac address of the interface
    if(ioctl(sockfd, SIOCGIFHWADDR, &req2) == -1){
        fprintf(stderr, "mac error");
        return(-1);
    }
    else{
        sprintf(sigInfo->mac, "%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[0]);
        for(i=1; i<6; i++){
            sprintf(sigInfo->mac+strlen(sigInfo->mac), ":%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[i]);
        }
    }
    close(sockfd);
}