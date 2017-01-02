#include "webserver.h" 
#include "ota.h"
#include "version.h"
#include "espressif/esp_common.h"
#include "config.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "http-parser/http_parser.h"
#include "page.h"
#include "main.h"

#define BACKLOG 10
#define MAX_POST_REQUEST_SIZE 2048

const char * path_start = NULL;
int path_length = 0;

const char * data_start = NULL;
int data_length = 0;

char* replace(char* str, char* a, char* b)
{
  int len  = strlen(str);
  int lena = strlen(a), lenb = strlen(b);
  char *p;
  for (p = str; (p = strstr(p, a)); ++p) {
    if (lena != lenb) // shift end as needed
      memmove(p+lenb, p+lena,
              len - (p - str) + lenb);
      memcpy(p, b, lenb);
    p+=strlen(b);
  }
  return str;
}

int create_and_bind() {
  int yes=1;
  struct sockaddr_in my_addr;
  int sockfd = 0;
  int portf = 80;
  
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return -1;
  }
  
  if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
    perror("setsockopt");
    return -1;
  }
  
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(portf);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(my_addr.sin_zero), '\0', 8);
  
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    return -1;
  }
    
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

static bool http_get_post_value(const char *_data, const char * _key, char *_output) {
  char start_key[64];
  const char * key_end = "\r\n--";
  if(sprintf(start_key, "name=\"%s\"\r\n\r\n", _key) > 0) {
    char * value_start = strstr(_data, start_key);
    if(value_start != NULL) {
      value_start += strlen(start_key);
      char * value_end = strstr(value_start, key_end);
      
      int value_len = value_end - value_start;
      memcpy(_output, value_start, value_len);
      _output[value_len] = '\0';
      return true;
    }
  }
  _output[0] = '\0';
  return false;
}

int my_url_callback (http_parser* _parser, const char *at, size_t length) {
  printf("my_url_callback\n");
  
  path_start = at;
  path_length = length;
  
  char format[10];
  memset(format, 0, 10);
  sprintf(format, "%%.%ds\n", length);
  
  printf(format,path_start);
  return 0;
}

int my_body_callback (http_parser* _parser, const char *at, size_t length) {
  printf("===BODY===========\n");
  
  data_start = at;
  data_length = length;
  
  char format[10];
  memset(format, 0, 10);
  sprintf(format, "%%.%ds\n", length);
  
//   printf(format,data_start);
  
  // really really basic security check
  bool check = false;
  uint32_t id = sdk_system_get_chip_id();
  char security[5];
  if( http_get_post_value(data_start, "security", security) ) {
    uint32_t _id = strtol(security, NULL, 10);
    if(id == _id) {
      check = true;
    }
  }
  if(! check) {
    return 0;
  }
  bool reset = false;
  char essid[33];
  char password[128];
  
  bool res = http_get_post_value(data_start, "essid", essid);
  res &= http_get_post_value(data_start, "password", password);
  
  
  if(res && essid[0] != '\0' && password[0] != '\0') {
    reset = save_network(essid, password);
  }
  
  char server[128];
  char port[5];
  
  res = http_get_post_value(data_start, "server", server);
  res &= http_get_post_value(data_start, "port", port);
  
  if(res && server[0] != '\0' && port[0] != '\0') {
    int port_number = strtol(port, NULL, 10);
    save_server(server, port_number);
    reset = true;
  }
  char upg[256];
  memset(upg, 0, 256);
  res = http_get_post_value(data_start, "upgrade", upg);
  if(res && strcmp(upg, "upgrade") == 0) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if(getpeername(*((int*)_parser->data), (struct sockaddr *)&addr, &addr_size) == 0 ) {
      const char * ip = inet_ntoa(addr.sin_addr);
      printf("fetching upg from %s\n", ip);
      ota_start(ip);
    } else {
      printf("Could not get client ip\n");
    }
  }
  
  if(reset) {
    printf("RESET\n");    
    sdk_system_restart();
  }
  
  return 0;
}

char buffer[MAX_POST_REQUEST_SIZE];
void handle(int _sockfd, struct sockaddr_in *_addr) {
  path_start = NULL;
  path_length = 0;
  
  data_start = NULL;
  data_length = 0;
  
  printf ("handle\n");
  memset(buffer, 0, MAX_POST_REQUEST_SIZE);
  int recved = read(_sockfd, buffer, MAX_POST_REQUEST_SIZE);
  if (recved < 0 ) {
    printf(" read error\n");
    return;
  }
  printf("received %d\n%s\n",recved,buffer);
  
  
//   https://github.com/nodejs/http-parser
  
  http_parser_settings settings = {
    /* on_message_begin */ 0,
    /* on_url */ 0,
    /* on_status */ 0,
    /* on_header_field */ 0,
    /* on_header_value */ 0,
    /* on_headers_complete */ 0,
    /* on_body */ 0,
    /* on_message_complete */ 0,
    /* on_chunk_header */ 0,
    /* on_chunk_complete */ 0
  };
  
  settings.on_url = my_url_callback;
  settings.on_body = my_body_callback;
  
  http_parser *parser = malloc(sizeof(http_parser));
  http_parser_init(parser, HTTP_REQUEST);
  parser->data = &_sockfd;
  
  
  /* Start up / continue the parser.
   * Note we pass recved==0 to signal that EOF has been received.
   */
  int nparsed = http_parser_execute(parser, &settings, buffer, recved);
  
  if (parser->upgrade) {
    /* handle new protocol */
  } else if (nparsed != recved) {
    /* Handle error. Usually just close the connection. */
  }
 
  int size = strlen(page_content)+128;
  char buffer[size];
  memset(buffer,0,size);
  sprintf(buffer, "%s", page_content);
  
  replace(buffer, "DATE_BUILD", BUILD_DATE);
  replace(buffer, "TIME_BUILD", BUILD_TIME);
  replace(buffer, "GIT_VERSION", GIT_VERSION);
  uint32_t id = sdk_system_get_chip_id();
  char id_buff[10];
  sprintf(id_buff, "%d", id);
  replace(buffer, "SERIAL", id_buff);
  
  struct sdk_station_config config;
  if(load_network(&config)) {
    replace(buffer, "ESSID", (char*)config.ssid);
  } else {
    replace(buffer, "ESSID", "NONE");
  }
  
  char server[256];
  memset(server, 0, 256);
  int port = 0;
  if(load_server(server, &port)) {
    
    replace(buffer, "SERVER", server);
    char tmp[5];
    sprintf(tmp, "%d", port);
    replace(buffer, "PORT", tmp);
  } else {
    replace(buffer, "SERVER", "0.0.0.0");
    replace(buffer, "PORT", "-1");
  }
  
  write(_sockfd, buffer, strlen(buffer));
  close(_sockfd);
}

void webserver_task(void *pvParameters) {
  int sockfd = create_and_bind();
  
  while(1) {
    socklen_t sin_size;
    int new_fd = 0;
    sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in their_addr;
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
      perror("accept");
      return;
    }
    handle(new_fd, &their_addr);
  }
}

void webserverInit() {
  printf("starting webserver\n");
  xTaskCreate(webserver_task, (const char *)"webserver_task", 1024, NULL, 3, NULL);//1024,866
}
