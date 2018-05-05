#define INBOUND_FIFO "INBOUND_FIFO"
#define OUTBOUND_FIFO "OUTBOUND_FIFO"
#define SOCKADDR "./tmp/socket1"

#include <sys/types.h>

#define COPY 0
#define PASTE 1
#define OK 2

typedef struct message {
  int action;
  int region;
  int size;
} message;

int clipboard_connect(char * clipboard_dir);
int clipboard_close(int clipboard_id);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
