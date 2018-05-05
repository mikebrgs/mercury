#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "clipboard.h"

typedef int bool;
#define false 0
#define true 1

int stay = 1;

typedef struct Connection {
  int fd;
  struct sockaddr_in addr;
} Connection;

char ** clipboard;
int size_clip[10];
pthread_mutex_t mutex_clip[10];
Connection backup;
bool use_backup = false;

void * clip_thread(void * arg) {
  int read_size;
  Connection this_client;
  this_client.fd = *(int*)arg;
  char * data = (char*)malloc(sizeof(message));
  message m;
  char * tmp_buffer;

  while ( (read_size = read(this_client.fd, data,
    sizeof(message))) > 0) {
    printf("read: %d\n", read_size);
    memcpy(&m, data, sizeof(message));
    if (m.action == COPY) {
      // Do copy
      printf("COPY\n");
      tmp_buffer = (char*) malloc(m.size);
      if (read(this_client.fd, tmp_buffer, m.size) <= 0) {
        continue;
      }
      pthread_mutex_lock(&mutex_clip[m.region]);
      if (clipboard[m.region] != NULL) {
        free(clipboard[m.region]);
      }
      clipboard[m.region] = (char*)malloc(m.size);
      memset(clipboard[m.region], '\0', m.size);
      memcpy(clipboard[m.region], tmp_buffer, m.size);
      printf("Copied: %s\n", clipboard[m.region]);
      size_clip[m.region] = m.size;
      pthread_mutex_unlock(&mutex_clip[m.region]);
      if (use_backup) {
        write(backup.fd, data, sizeof(message));
        write(backup.fd, tmp_buffer, m.size);
      }
      free(tmp_buffer);
      printf("Sent to backup\n");
    } else if (m.action == PASTE) {
      // Do paste
      printf("PASTE\n");
      pthread_mutex_lock(&mutex_clip[m.region]);
      m.size = size_clip[m.region];
      memcpy(data, &m, sizeof(message));
      write(this_client.fd, data, sizeof(message));
      write(this_client.fd, clipboard[m.region], size_clip[m.region]);
      pthread_mutex_unlock(&mutex_clip[m.region]);
      printf("Pasted: %s - %d\n", clipboard[m.region], size_clip[m.region]);
    } else {
      printf("UNCONVENTIONAL %d\n", m.action);
    }
  }
  close(this_client.fd);
  printf("Client left\n");
  return NULL;
}

void hander(int dummy) {
  if (use_backup)
    close(backup.fd);
  exit(0);
}

int main(int argc, char ** argv){

  char file_name[100];
  struct sockaddr_un clip_addr;
  clip_addr.sun_family = AF_UNIX;
  strcpy(clip_addr.sun_path, SOCKADDR);

  backup.fd = socket(AF_INET, SOCK_STREAM, 0);
  backup.addr.sin_family = AF_INET;
  bool use_backup = false;
  int backup_addrlen;
  if (argc >= 4) {
    if(strcmp(argv[1], "-c") == 0
      && inet_aton(argv[2], &(backup.addr.sin_addr)) != 0) {
      use_backup = true;
      backup.addr.sin_port = htons(atoi(argv[3]));
      backup_addrlen = sizeof(backup.addr);
    } else {
      use_backup = false;
    }
  }


  unlink(SOCKADDR);
  int clip_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (clip_fd == -1) {
    printf("error_socket\n");
    return -1;
  }

  if (bind(clip_fd, (struct sockaddr*)&clip_addr,
    sizeof(clip_addr)) == -1) {
    perror("error: ");
    printf("error_bind\n");
    return -1;
  }

  if (listen(clip_fd, 5) == -1) {
    printf("error_listen\n");
    return -1;
  }

  //abrir FIFOS
  int client_fd;
  int clip_addr_len = sizeof(clip_addr);
  clipboard = (char**)malloc(10*sizeof(char*));
  for (int i = 0; i < 10; i++) {
    clipboard[i] = NULL;
    size_clip[i] = 0;
    if (pthread_mutex_init(&mutex_clip[i], NULL) != 0) {
      return -1;
    }
  }
  char * data = (char*)malloc(sizeof(message));
  message m;

  if (use_backup == true
    && connect(backup.fd, (struct sockaddr*)&(backup.addr),
    sizeof(backup.addr)) == -1) {
    printf("error connecting\n");
    return -1;
  }

  if (use_backup == true) {
    char * tmp_buffer;
    for (int i = 0; i < 10; i++) {
      m.region = i;
      m.action = PASTE;
      memcpy(data, &m, sizeof(message));
      write(backup.fd, data, sizeof(message));
      read(backup.fd, data, sizeof(message));
      memcpy(&m, data, sizeof(message));
      tmp_buffer = (char*)malloc(m.size);
      read(backup.fd, tmp_buffer, m.size);
      printf("size: %d\n", m.size);
      clipboard[i] = (char*)malloc(m.size);
      memset(clipboard[i], '\0', m.size);
      memcpy(clipboard[i], tmp_buffer, m.size);
      size_clip[i] = m.size;
      free(tmp_buffer);
    }
  }

  while(stay){
    if ( (client_fd = accept(clip_fd,
      (struct sockaddr*)&clip_addr, &clip_addr_len )) == -1) {
      perror("accept: ");
      continue;
    }
    printf("New client\n");
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, clip_thread, &client_fd);
  }

  exit(0);

}
