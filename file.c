/* file.c
 *
 * simple File i/o routines.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "file.h"

int
file_size(const char *fname) {
  struct stat fbuf;
  if (stat(fname, &fbuf)) {
    return -1;
  }

  return fbuf.st_size;
}

char *
file_read(const char *fname) {
  int size = file_size(fname);
  char *buff;
  FILE *fin;
  if (size < 0) return NULL;
  if (size == 0) return "";
  if (size > MAX_FILE_SIZE) {
    return "MAXIMUM FILE SIZE EXCEEDED.";
  }
  // size + 1, since we add a null character. If anyone wants the actual 
  // file size, that's what file_size is for.
  fin = fopen(fname, "r");
  if (fin == NULL) {
    return NULL;
  }

  buff = malloc(size + 1);
  if ((int) fread(buff, 1, size, fin) != size) {
    free(buff);
    return NULL;
  }
  buff[size] = '\0';
  return buff;
}

int
file_write(const char *fname, const char *contents) {
  return 0;
}
