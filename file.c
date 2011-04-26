/* file.c
 *
 * simple File i/o routines.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"

/** Read a number from a file and return it, if it exists. If it doesn't,
 *  the default is returned.
 */
int
file_readnum(const char *fname, int def) {
  char *s;
  int ret;
  if (!file_exists(fname))
    return def;

  s = file_read(fname);
  if (!s) return def;
  ret = atoi(s);
  free(s);
  return ret;
}

int
file_writenum(const char *fname, int val) {
  char buff[10];

  snprintf(buff, 10, "%d\n", val);
  return file_write(fname, buff);
}

/**
 * yorn: Yes or no. return 1 if the contents of a given file
 * start with "yes", "1", "enabled" or "true". Otherwise return 0.
 */
int
file_yorn(const char *fname) {
  char *s;
  char buff[10];
  if (!file_exists(fname))
    return 0;

  s = file_read(fname);
  if (!s) return 0;
  snprintf(buff, 10, "%s", s);
  free(s);
  buff[9] = '\0';

  if (!strncmp(buff, "y", 1)) { return 1; }
  if (!strncmp(buff, "1", 1)) { return 1; }
  if (!strncmp(buff, "true", 4)) { return 1; }
  if (!strncmp(buff, "enabled", 7)) { return 1; }
  // No other currently valid options.
  return 0;
}

int
file_size(const char *fname) {
  struct stat fbuf;
  if (stat(fname, &fbuf)) {
    return -1;
  }

  return fbuf.st_size;
}

/** Read a file into memory. It must be smaller than MAX_FILE_SIZE, and
 * must exist. Any non-NULLs returned by this pointer must be free()'d.
 */
char *
file_read(const char *fname) {
  int size = file_size(fname);
  char *buff;
  FILE *fin;
  if (size < 0) return NULL;
  if (size == 0) return strdup("");
  if (size > MAX_FILE_SIZE) {
    return strdup("MAXIMUM FILE SIZE EXCEEDED.");
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
