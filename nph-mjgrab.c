/*  Kenneth Lavrsen's jpeg grabber for Motion: nph-mjgrab
 *  This program is published under the GNU Public license
 *  Version 1.0 - 2003 November 19
 *  
 *  The program opens a socket to a running Motion webcam server
 *  It then fetches ONE and only one frame of the mjpeg stream
 *  and sends it to standard out (browser) after having sent a few headers.
 *  It then closes the socket and terminates.
 *  The idea is to show a snapshot of a camera without saving any files
 *  on the web server.
 *  This program is made as a cgi program for e.g. an Apache server.
 *  It runs as a nph (direct to browser) program. It takes one variable
 *  which is the camera (Motion thread number).
 *  The program must be copied to a directory from which cgi scripts can run.
 *  It must be have access rights set as executable for the web server.
 *  The program must be named with the prefix nph- .
 *  The cgi program is run by making an image tag in a HTML file like this:
 *  <IMG SRC = "/cgi-bin/nph-mjgrab?1" WIDTH=320 HEIGHT=240>
 *  Note the number after the "?". This is the camera number (Motion thread no.)
 *  Below the 3 #defines must be set before you build the program.
 *  You need to set IP address of the motion server, the portbase
 *  and the upper limit for the number of cameras you have.
 *  IP address 127.0.0.1 means local host (same machine).
 *  PORTBASE is the number from which the webcam port is calculated.
 *  The calculated port = PORTBASE + Camera Number (thread number).
 *  So if thread 1 has the webcam port set to 8081 the PORTBASE should be 8080
 *  Program can be built with this command:
 *  gcc -Wall -O3 -o nph-mjgrab nph-mjgrab.c
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

/*  User defined parameters - EDIT HERE */
#define IPADDR            "127.0.0.1"  /* IP address of motion server, 127.0.0.1 = localhost */
#define PORTBASE          8080         /* If your camera 1 is on port 8081 set this to one lower = 8080 */
#define UPPERCAMERANUMBER 10           /* Set this to your upper limit of cameras */

int main(void)
{
    struct  sockaddr_in address;
    int     sockfd           = 0;
    FILE   *fd               = NULL;
    int     len              = 0;
    int     result           = 0;
    int     jpglength        = 0;
    int     cameranumber     = 0;
    char    garbage1[80]     = "";
    char    garbage2[30]     = "";
    char    chbuffer[262144] = "";
    char   *querystr         = NULL;
    char   *fgets_result     = NULL;

/*  Get camera number from querystring  */

    querystr = getenv("QUERY_STRING");
    sscanf(querystr,"%d",&cameranumber);

    if (cameranumber<1 || cameranumber>UPPERCAMERANUMBER)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, invalid camera number %i\n", cameranumber, cameranumber);
        exit(1);
    }

    /*  Create a socket for the client.  */
    errno = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == 0 || errno != 0)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, create socket: %s\n", cameranumber, strerror(errno));
        perror("Unable to create socket file descriptor...");
        exit(1);
    }

    /*  Name the socket, as agreed with the server.  */
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = inet_addr(IPADDR);
    address.sin_port        = htons(PORTBASE + cameranumber);
    len                     = sizeof(address);

    /*  Now connect our socket to the server's socket.  */
    errno = 0; 
    result = connect(sockfd, (struct sockaddr *)&address, len);
    if(result == -1 || errno != 0)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, unable to connect to port %i: %s\n", cameranumber, PORTBASE+cameranumber, strerror(errno));
        perror("oops: Cannot connect to Motion(1)");
        close(sockfd);
        exit(1);
    }
   
    errno = 0; 
    fd = fdopen(sockfd,"r");
    if(fd == NULL || errno != 0)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, unable to open socket for reading: %s\n", cameranumber, strerror(errno));
        perror("oops: Cannot connect to Motion(2)");
        close(sockfd);
        exit(1);
    }
    
    /*  We can now read/write via sockfd. */
    while(strncmp(garbage1,"Content-Length:",15)!=0)
    {
        fgets_result = fgets(garbage1, 70, fd);
        if(fgets_result == NULL)
        {
            fprintf(stderr, "nph-mjgrab?%i failed, had trouble with first fgets: %s\n", cameranumber, strerror(errno));
            perror("oops: nph-mjgrab had trouble with fgets(1)");
            fclose(fd);
            close(sockfd);
            exit(1);
        }
    }
    
    sscanf(garbage1,"%s%d",garbage2,&jpglength);
    if(jpglength > sizeof(chbuffer))
    {
        fprintf(stderr, "nph-mjgrab?%i failed: jpglength = %i, max allowed = %i\n", cameranumber, jpglength, sizeof(chbuffer));
        fclose(fd);
        close(sockfd);
        exit(1);
    }
   
    errno = 0; 
    fgets_result = fgets(garbage1, 70, fd);
    if(fgets_result == NULL)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, had trouble with second fgets: %s\n", cameranumber, strerror(errno));
        fclose(fd);
        close(sockfd);
        exit(1);
    }

    errno = 0; 
    result = fread(chbuffer, 1, jpglength, fd);
    if(result==0)
    {
        fprintf(stderr, "nph-mjgrab?%i failed, had trouble with fread: %s\n", cameranumber, strerror(errno));
        fclose(fd);
        close(sockfd);
        exit(1);
    }

    printf("HTTP/1.1 200 OK\n");
    printf("Content-Type: image/jpeg\n\n");
    fwrite(chbuffer, 1, jpglength, stdout);
    fflush(stdout);
    
    fclose(fd);
    close(sockfd);
    
    exit(0);
}
