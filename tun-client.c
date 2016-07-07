#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

#include <fcntl.h>  /* O_RDWR */
//#include <string.h> /* memset(), memcpy() */
//#include <stdio.h> /* perror(), printf(), fprintf() */
#include <stdlib.h> /* exit(), malloc(), free() */
#include <sys/ioctl.h> /* ioctl() */

/* includes for struct ifreq, etc */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_open(char *devname)
{
    struct ifreq ifr;
    int fd, err;

    if ( (fd = open("/dev/net/tun", O_RDWR)) == -1 ) {
        perror("open /dev/net/tun");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = (IFF_TUN | IFF_NO_PI);
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    /* ioctl will use if_name as the name of TUN
    * interface to open: "tun0", etc. */
    if ( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) == -1 ) {
        perror("ioctl TUNSETIFF");close(fd);exit(1);
    }

    /* After the ioctl call the fd is "connected" to tun device
    * specified
    * by devname */

    return fd;
}

int main(int argc , char *argv[])
{
    //-------------------------------------------------------------------------
    // Prepare the connection to the 'asa1' TUN interface first.
    int fd, nbytes;
    char buf[1600];

    // opening first TUN interface
    fd = tun_open("asa1") ;
    printf("Device asa1 opened\n");

    //-------------------------------------------------------------------------
    // preparing the TCP connection to the tun-router-server
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("172.16.215.130");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
     
    //keep communicating with server
    while(1)
    {
        // read from the TUN interface file descriptor      
        nbytes = read(fd, buf, sizeof(buf)); 
        printf("Read %d bytes from asa1\n", nbytes);

        // send to the remote TCP server
        if( send(sock, buf, nbytes, 0) < 0)
        {
            puts("Send failed");
            return 1;
        }
         
        //Receive a reply from the remote TCP server
        nbytes = 0;
        if( (nbytes = recv(sock , server_reply , 2000 , 0)) < 0)
        {
            puts("recv failed");
            break;
        }
         
        printf("Server replied with %d bytes\n", nbytes);
        //puts(server_reply);

        // should write the data back to the TUN interface file descriptor
        nbytes = write(fd, server_reply, nbytes);
        //nbytes = write(fd, buf, nbytes);
        printf("Wrote %d bytes to asa1\n", nbytes);

        memset(message, 0, sizeof(message));
        memset(server_reply, 0, sizeof(server_reply));

        //break;
    }
     
    close(sock);
    return 0;
}
