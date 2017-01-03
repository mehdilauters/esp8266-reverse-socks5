#include "reverse_tcp.h" 
#include <string.h>
#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <netdb.h>


char m_server[256];
char m_client[256];
int m_port;
int m_port_client;

int create_socket(char *_server, int _port) {
  int sockfd;
  struct sockaddr_in serverSockAddr;
  struct hostent *serverHostEnt;
  long hostAddr;
  bzero(&serverSockAddr,sizeof(serverSockAddr));
  hostAddr = inet_addr(m_server);
  if ( (long)hostAddr != (long)-1)
    bcopy(&hostAddr,&serverSockAddr.sin_addr,sizeof(hostAddr));
  else
  {
    serverHostEnt = gethostbyname(_server);
    if (serverHostEnt == NULL)
    {
      printf("gethost fail\n");
      return -1;
    }
    bcopy(serverHostEnt->h_addr,&serverSockAddr.sin_addr,serverHostEnt->h_length);
  }
  serverSockAddr.sin_port = htons(_port);
  serverSockAddr.sin_family = AF_INET;
  
  if ( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    printf("failed to create socket\n");
    return -1;
  }
  
  if(connect( sockfd,
    (struct sockaddr *)&serverSockAddr,
              sizeof(serverSockAddr)) < 0 ) {
    printf("conection failed\n");
    return -1;
  }
  return sockfd;
}

char server_buffer[256];
char client_buffer[256];
void reverse_tcp_task(void *pvParameters) {
  while(1) {
    
    int sockfd_server = create_socket(m_server, m_port);
    int sockfd_client = create_socket(m_client, m_port_client);
    
    int max = sockfd_client;
    if(sockfd_server > sockfd_client) {
      max = sockfd_server; 
    }
    
    if(!(sockfd_client < 0 || sockfd_server < 0)) {
      printf("Connected\n");
      int ret;
      fd_set readfs;
      FD_ZERO(&readfs);
      FD_SET(sockfd_server, &readfs);
      FD_SET(sockfd_client, &readfs);
      while(true) {
        printf("...\n");
        if((ret = select(max + 1, &readfs, NULL, NULL, NULL)) > 0) {
          printf("!\n");
          if(FD_ISSET(sockfd_client, &readfs)) {
            printf("Client available\n");
            int res = read(sockfd_client, client_buffer, 1);
            if(res > 0) {
              write(sockfd_server, client_buffer, res);
              printf("client=>server\n");
            }
          }
          if(FD_ISSET(sockfd_server, &readfs)) {
            printf("Server available\n");
            int res = read(sockfd_server, server_buffer, 1);
            if(res > 0) {
              write(sockfd_client, server_buffer, res);
              printf("server=>client\n");
            }
          }
        }
      }
    }
    
    vTaskDelay(100);
  }
}


void start_reverse_tcp(const char * _server, int _port, const char * _client, int _client_port) {
  memcpy(m_server, _server, strlen(_server));
  m_port = _port;
  
  memcpy(m_client, _client, strlen(_client));
  m_port_client = _client_port;
  
  printf("Tunneling %s:%d <=> %s:%d\n", m_server, m_port, m_client, m_port_client);
  xTaskCreate(reverse_tcp_task, (const char *)"reverse_tcp_task", 512, NULL, 1, NULL);//1024,866
}