#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
  int fd;
  unsigned char buf[16];
  int i;
  int start = 0;
  int size = 0;
  
  if (argc < 2) {
    fprintf(stderr, "usage %s rawfile\n", argv[0]);
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    fprintf(stderr, "can't open %s\n", argv[1]);
    exit(1);
  }

  printf("// Disk Image\n");
  printf("#include \"usbd_storage_if.h\"\n\n");
  printf("unsigned char DiskImage[MSC_MemorySize] = {\n");

  int len;
  while (0<(len = read(fd,buf,sizeof(buf)))) {
    size += len;
    int i;
    if (start)
      printf(",\n");
    start = 1;
    for (i = 0; i < sizeof(buf); i++) {
      printf("%s0x%02x",i?",":"",buf[i]);
    }
  }
  printf("};\n");
  fprintf(stderr,"size = %d\n", size);
}
