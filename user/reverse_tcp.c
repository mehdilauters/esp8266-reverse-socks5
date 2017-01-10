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
int sockfd_server = -1;
int sockfd_client = -1;

TaskHandle_t server_client_task = NULL;
TaskHandle_t client_server_task = NULL;
bool running;

void reverse_tcp_stop() {
  
  if(server_client_task != NULL) {
    vTaskDelete( server_client_task );
  }
  
  if(client_server_task != NULL) {
    vTaskDelete( client_server_task );
  }
  
  server_client_task = NULL;
  client_server_task = NULL;
  
  if(sockfd_client != -1) {
    close(sockfd_client);
  }
  
  if(sockfd_server != -1) {
    close(sockfd_server);
  }
  
  sockfd_client = -1;
  sockfd_server = -1;
}

void reverse_tcp_client_to_server(void *pvParameters) {
  while(true) {
    int res = read(sockfd_client, client_buffer, sizeof(client_buffer));
    if(res > 0) {
      write(sockfd_server, client_buffer, res);
      vTaskDelay(100 / portTICK_PERIOD_MS );
    } else {
      printf("client read error\n");
      vTaskDelay(100);
      reverse_tcp_stop();
    }
  }
}

void reverse_tcp_server_to_client(void *pvParameters) {
  while(true) {
    memset(server_buffer, 0, sizeof(server_buffer));
    int res = read(sockfd_server, server_buffer, sizeof(server_buffer));
    if(res > 0) {
      if(sockfd_client < 0) {
        char * separator = strstr(server_buffer, ":");
        if(separator != NULL) {
          int value_len = separator - server_buffer;
          memcpy(m_client, server_buffer, value_len);
          m_client[value_len] = '\0';
          char *p = server_buffer + value_len + 1; // ":" length
          m_port_client = strtol(p, NULL, 10);
          sockfd_client = create_socket(m_client, m_port_client);
          if(sockfd_client < 0) {
            printf("Could not connect to client\n");
          } else {
            printf("Tunneling %s:%d <=> %s:%d\n", m_server, m_port, m_client, m_port_client);
            xTaskCreate(reverse_tcp_client_to_server, (const char *)"reverse_tcp_client_to_server", 512, NULL, 1, &client_server_task);
          }
        }
      } else {
        write(sockfd_client, server_buffer, res);
      }
      vTaskDelay(100 / portTICK_PERIOD_MS );
    } else {
      printf("server read error\n");
      running = false;
      vTaskDelay(100);
    }
  }
}

void reverse_tcp_task(void *pvParameters) {
  while(1) {
    running = true;
    sockfd_server = create_socket(m_server, m_port);
    bool start = true;
    if(sockfd_server < 0) {
      printf("Could not connect to server\n");
      start = false;
    }
    
    if(start) {
      printf("starting...\n");
      xTaskCreate(reverse_tcp_server_to_client, (const char *)"reverse_tcp_server_to_client", 512, NULL, 1, &server_client_task);
      while(running) vTaskDelay(100);
      reverse_tcp_stop();
      vTaskDelay(100);
    }
    
    
//     sockfd_client = create_socket(m_client, m_port_client);
//     if(sockfd_client < 0) {
//       printf("Could not connect to server\n");
//       if(sockfd_server >= 0) {
//         close(sockfd_server);
//       }
//       start = false;
//     }
//     
//     if(start) {
//       xTaskCreate(reverse_tcp_client_to_server, (const char *)"reverse_tcp_client_to_server", 512, NULL, 1, &client_server_task);
//       reverse_tcp_stop();
//     }
    vTaskDelay(1000   / portTICK_PERIOD_MS);
  }
}


void start_reverse_tcp(const char * _server, int _port) {
  memcpy(m_server, _server, strlen(_server));
  m_port = _port;
  xTaskCreate(reverse_tcp_task, (const char *)"reverse_tcp_task", 512, NULL, 1, NULL);
}