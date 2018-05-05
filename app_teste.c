#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>

int fd;

void handler(int dummy) {
  clipboard_close(fd);
  exit(0);
}

int main(){
  signal(SIGINT, handler);
  printf("connect\n");
  fd = clipboard_connect(SOCKADDR);
  if(fd== -1){
    exit(-1);
  }

  char dados_in[1024];
  char dados_out[1024];
  char dados_cmds[1024];
  char cmd[1024];
  int zone;
  while(1) {
    fgets(dados_cmds, 1024, stdin);
    sscanf(dados_cmds, "%s %d[^\n]\n", cmd, &zone);
    if (strcmp(cmd, "copy") == 0
      && zone >= 0 && zone < 10) {
        memset(&dados_in, '\0', 1024);
        fgets(dados_in, 1024, stdin);
        printf("copy\n");
        clipboard_copy(fd, zone, (void*)dados_in, strlen(dados_in));
    } else if (strcmp(cmd, "paste") == 0
      && zone >= 0 && zone < 10) {
        printf("paste\n");
        memset(&dados_out, '\0', 1024);
        clipboard_paste(fd, zone, (void*)dados_out, 1024);
        printf("Received %s - %d\n", dados_out, (int)strlen(dados_out));
    } else {
      printf("invalid\n");
    }
  }
  exit(0);
  }
