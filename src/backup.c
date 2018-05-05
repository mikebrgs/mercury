#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include "clipboard.h"

typedef struct Connection {
  int fd;
  struct sockaddr_in addr;
} Connection;

int main(int argc, char ** argv) {
  // if (argc < 2) {
  //   printf("./backup\n");
  //   return -1;
  // }

  Connection server, client;
  server.fd = socket(AF_INET, SOCK_STREAM, 0);
  server.addr.sin_family = AF_INET;
  server.addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server.addr.sin_port = htons(37102);

  bind(server.fd, (struct sockaddr*)&(server.addr),
    sizeof(server.addr));
  listen(server.fd, 5);

  char ** clipboard;
  int size_clip[10];
  clipboard = (char**)malloc(10*sizeof(char*));
  for (int i = 0; i < 10; i++) {
    clipboard[i] = NULL;
    size_clip[i] = 0;
  }

  int clientlen;
  message m;
  char m_buffer[sizeof(message)];
  char * tmp_buffer;
  while (1) {
    client.fd = accept(server.fd, (struct sockaddr*)&(client.addr), &clientlen);
    printf("accepted\n");
    while (read(client.fd, &m_buffer, sizeof(message)) > 0) {
      printf("reading\n");
      memcpy(&m, m_buffer, sizeof(message));
      if (m.action == COPY) {
        // Do copy
        printf("back-up region: %d size: %d\n", m.region, m.size);
        if (clipboard[m.region] != NULL) {
          free(clipboard[m.region]);
        }
        clipboard[m.region] = (char*)malloc(m.size);
        memset(clipboard[m.region], '\0', m.size);
        read(client.fd, clipboard[m.region], m.size);
        printf("Copied: %s\n", clipboard[m.region]);
        size_clip[m.region] = m.size;
      } else if (m.action == PASTE) {
        // Do paste
        printf("initialization: %d\n", m.region);
        m.size = size_clip[m.region];
        memcpy(m_buffer, &m, sizeof(message));
        write(client.fd, m_buffer, sizeof(message));
        write(client.fd, clipboard[m.region], size_clip[m.region]);
        printf("Pasted: %s\n", clipboard[m.region]);
      } else {
        printf("UNCONVENTIONAL %d\n", m.action);
      }
    }
    close(client.fd);
  }

}
