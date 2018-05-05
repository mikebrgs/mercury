#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int clipboard_connect(char * clipboard_dir){
  char fifo_name[100];
  struct sockaddr_un clip_addr;

  clip_addr.sun_family = AF_UNIX;
  strcpy(clip_addr.sun_path, clipboard_dir);

  int clip_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (connect(clip_fd, (struct sockaddr*)&clip_addr,
    sizeof(clip_addr)) == -1) {
    return -1;
  }
  return clip_fd;
}

int clipboard_close(int clipboard_id) {
  close(clipboard_id);
  return 0;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
  int n;
  message m;
  m.action = COPY;
  m.region = region;
  m.size = count;
  char * msg = (char*)malloc(sizeof(message));
  memcpy(msg, &m, sizeof(message));
  n = write(clipboard_id, msg, sizeof(message));
  if (n <= 0) {
    perror("write() 1");
    return -1;
  }
  n = write(clipboard_id, buf, count);
  printf("clipboard_copy.write: %d - region %d\n", n, m.region);
  if (n <= 0) {
    perror("write() 2");
    return -1;
  }
  free(msg);
  return count;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
  int n;
  printf("clipboard_paste\n");
  message m;
  m.action = PASTE;
  m.region = region;
  char * msg = (char*)malloc(sizeof(message));
  memcpy(msg, &m, sizeof(message));
  // Send request
  n = write(clipboard_id, msg, sizeof(message));
  if (n <= 0) {
    perror("write() ");
    return -1;
  }
  printf("clipboard_paste.write %d\n", n);
  // receive response
  n = read(clipboard_id, msg, sizeof(message));
  if (n <= 0) {
    perror("read() ");
    return -1;
  }
  memcpy(&m, msg, sizeof(message));
  printf("clipboard_paste.read1 %d - %d\n",m.size, m.region);
  char * paste_buffer = (char*)malloc(sizeof(char)*m.size);
  n = read(clipboard_id, paste_buffer, m.size);
  if (n <= 0) {
    perror("read() ");
    return -1;
  }
  printf("clipboard_paste.read2 %s - %d\n",paste_buffer, n);
  if (m.size > 1024) {
    memcpy(buf, paste_buffer, 1024);
  } else {
    memcpy(buf, paste_buffer, m.size);
  }
  free(msg);
  return m.size;
}
