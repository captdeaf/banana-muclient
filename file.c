/* file.c
 *
 * simple File i/o routines.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"

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
    fclose(fin);
    return NULL;
  }
  fclose(fin);
  buff[size] = '\0';
  return buff;
}

int
file_write_len(const char *fname, const char *contents, int len) {
  FILE *fout;

  int ret;

  fout = fopen(fname, "w");
  if (fout == NULL) {
    return 1;
  }

  ret = (int) fwrite(contents, 1, len, fout);
  if (ret < len) {
    ret = ferror(fout);
    fclose(fout);
    return ret;
  }
  fclose(fout);
  return 0;
}

int
file_write(const char *fname, const char *contents) {
  return file_write_len(fname, contents, strlen(contents));
}  

int
file_mkdir(char *pathname) {
  int ret;
  char pbuff[200];
  char *p;
  ret = mkdir(pathname, 0755);
  if (ret) {
    if (errno == EEXIST) { return 0; }
    if (errno == ENOENT) {
      snprintf(pbuff, 200, "%s", pathname);
      p = strrchr(pbuff, '/');
      if (p) {
        *(p) = '\0';
        file_mkdir(pbuff);
        return mkdir(pathname, 0777);
      }
    }
  }
  return 0;
}
