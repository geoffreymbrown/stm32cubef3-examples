#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>



int fd;
volatile int sent = 0;
int rcvd = 0;
volatile int done = 0;

void *inthd(void *arg) {
  char buf[256];  
  int cnt;
  
  while (0 < (cnt = read(fileno(stdin), buf, sizeof(buf)))) {
    int len = 0;
    while (cnt) {
      if (0<(len = write(fd,buf,cnt))) {
	sent += len;
	cnt  -= len;
      } ;
      if (len == 0) 
	break;
      if (len < 0) {
	if (errno == EAGAIN)
	  continue;
	else {
	  perror("tty write error:");
	  break;
	}
      }
    }
    if (cnt) {
      fprintf(stderr, "exiting with %d bytes unsent\n", cnt);
      break;
    } 
  }
  fprintf(stderr, "sent %d bytes \n", sent);
  done = 1;
  return NULL;
}


int main(int argc, char *argv[]) {
  pthread_t child;

  if (argc != 2){
      fprintf(stderr,"usage: %s device\n", argv[0]);
      return 1;
    }

  fd = open(argv[1], O_RDWR | O_NDELAY | O_NOCTTY);

  if (fd == -1){
      perror("can't open device");
      fprintf(stderr,"%s\n", argv[1]);
      exit(errno);
    }

  if (fcntl(fd, F_SETFL, 0) < 0)    {
    printf("Error clearing O_NONBLOCK\n");
    exit(errno);
    }

  struct termios config, orig_config;
  if (tcgetattr(fd, &config) < 0) {
      perror("can't get serial attributes");
      exit(errno);
    }

  orig_config = config;

  cfmakeraw(&config);
  config.c_cc[VMIN] = 1;
  config.c_cc[VTIME] = 10;
  config.c_cflag |= CLOCAL | CREAD;


  if (tcsetattr(fd, TCSANOW, &config) < 0) {
      perror("can't set serial attributes");
      exit(errno);
    }

  tcflush(fd, TCIOFLUSH);

  char buf[256];  
  int cnt;

  if (pthread_create(&child, NULL, inthd,NULL)) {
    perror("thread creation problem");
    close(fd);
    exit(0);
  }
  
  while ((!done || (rcvd < sent)) &&  (0 < (cnt = read(fd, buf, sizeof(buf))))) {
    int len = 0;
    rcvd += cnt;
    while (cnt) {
      if (0<(len = write(fileno(stdout),buf,cnt))) {
	cnt  -= len;
      } ;
      if (len == 0) 
	break;
      if (len < 0) {
	if (errno == EAGAIN)
	  continue;
	else {
	  perror(" write error:");
	  break;
	}
      }
    }
    if (cnt) {
      fprintf(stderr, "exiting with %d bytes unsent\n", cnt);
      break;
    } 
  }
  fprintf(stderr, "received %d bytes \n", rcvd);
  close(fd);
}
