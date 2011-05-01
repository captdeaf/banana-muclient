/* file.c
 *
 * simple File i/o routines.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"
#include <dirent.h>

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

// Anything returned by this needs to be free()'d.
char *
file_mimetype(const char *fpath) {
  char mimebuff[300];
  FILE *fio;

  fio = popen("./get_mtype.sh","r+");
  if (!fio) return strdup("text/plain");
  fwrite(fpath, 1, strlen(fpath), fio);
  fwrite("\n", 1, 1, fio);
  fflush(fio);
  slog("Getting mimetype for %s", fpath);
  fgets(mimebuff, 300, fio);
  slog("Getting mimetype for %s: %s", fpath, mimebuff);
  pclose(fio);
  return strdup(mimebuff);
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

void
send_file_data(struct mg_connection *conn, FILE *fp, int64_t len) {
  char buf[1024];
  int to_read, num_read, num_written;

  while (len > 0) {
    // Calculate how much to read from the file in the buffer
    to_read = sizeof(buf);
    if ((int64_t) to_read > len)
      to_read = (int) len;

    // Read from file, exit the loop on error
    if ((num_read = fread(buf, 1, (size_t)to_read, fp)) == 0)
      break;

    // Send read bytes to the client, exit the loop on error
    if ((num_written = mg_write(conn, buf, (size_t)num_read)) != num_read)
      break;

    // Both read and were successful, adjust counters
    len -= num_written;
  }
}

void
file_read_to_conn(struct mg_connection *conn, const char *path, int dohead) {
  char date[64], lm[64], etag[64], range[64];
  const char *fmt = "%a, %d %b %Y %H:%M:%S %Z", *msg = "OK", *hdr;
  time_t curtime = time(NULL);
  int cl, r1, r2;
  FILE *fp;
  int n;
  int status_code = 200;
  char *mtype;
  struct stat fbuf;

  if (stat(path, &fbuf)) {
    send_error(conn, strerror(errno));
    return;
  }

  cl = fbuf.st_size;

  range[0] = '\0';

  mtype = file_mimetype(path);

  if ((fp = fopen(path, "rb")) == NULL) {
    free(mtype);
    send_error(conn, "Unable to determine mime-type");
    return;
  }

  // If Range: header specified, act accordingly
  r1 = r2 = 0;
  hdr = mg_get_header(conn, "Range");
  if (hdr != NULL && (n = sscanf(hdr, "bytes=%I64d-%I64d", &r1, &r2)) > 0) {
    status_code = 206;
    (void) fseeko(fp, (off_t) r1, SEEK_SET);
    cl = n == 2 ? r2 - r1 + 1: cl - r1;
    (void) snprintf(range, sizeof(range),
        "Content-Range: bytes "
        "%I64d-%I64d/%I64d\r\n",
        r1, r1 + cl - 1, (int) fbuf.st_size);
    msg = "Partial Content";
  }

  // Prepare Etag, Date, Last-Modified headers
  strftime(date, sizeof(date), fmt, localtime(&curtime));
  strftime(lm, sizeof(lm), fmt, localtime(&fbuf.st_mtime));
  snprintf(etag, sizeof(etag), "%lx.%lx",
      (unsigned long) fbuf.st_mtime, (unsigned long) fbuf.st_size);

  (void) mg_printf(conn,
      "HTTP/1.1 %d %s\r\n"
      "Date: %s\r\n"
      "Last-Modified: %s\r\n"
      "Etag: \"%s\"\r\n"
      "Content-Type: %.*s\r\n"
      "Content-Length: %I64d\r\n"
      "Connection: %s\r\n"
      "Accept-Ranges: bytes\r\n"
      "%s\r\n",
      status_code, msg, date, lm, etag,
      strlen(mtype), mtype, cl, "keep-alive", range);

  if (!dohead) {
    send_file_data(conn, fp, cl);
  }
  (void) fclose(fp);
  free(mtype);
}

void
file_list_to_conn(struct mg_connection *conn, const char *path) {
  DIR *dp;
  struct dirent *ep;
  struct stat fbuf;
  const char *p;
  int count;

  char date[64], lm[64];
  const char *fmt = "%a, %d %b %Y %H:%M:%S %Z";
  time_t curtime = time(NULL);

  if (stat(path, &fbuf)) {
    send_error(conn, strerror(errno));
    return;
  }

  if (!(fbuf.st_mode & S_IFDIR)) {
    send_error(conn, "Requested path is not a directory.");
    return;
  }

  dp = opendir(path);

  if (dp == NULL) {
    send_error(conn, "Unable to determine mime-type");
    return;
  }

  // Prepare Etag, Date, Last-Modified headers
  strftime(date, sizeof(date), fmt, localtime(&curtime));
  strftime(lm, sizeof(lm), fmt, localtime(&fbuf.st_mtime));

  // Send the header.
  (void) mg_printf(conn,
      "HTTP/1.1 200 OK\r\n"
      "Date: %s\r\n"
      "Last-Modified: %s\r\n"
      "Content-Type: text/html\r\n"
      "Connection: keep-alive\r\n"
      "\r\n",
      date, lm);

  p = strrchr(path, '/');
  if (p && *(p + 1)) {
    p++;
  } else {
    p = path;
  }
  (void) mg_printf(conn,
      "<HTML>\r\n"
      "<HEAD><TITLE>Directory listing for %s</TITLE></HEAD>\r\n"
      "<BODY>\r\n"
      "<h3>Directory listing for %s</h3><br />\r\n"
      ,
      p, p);

  count = 0;
  while ((ep = readdir(dp)) != NULL) {
    if (ep->d_name[0] != '.') {
      // We don't print hidden files.
      mg_printf(conn, "<a href=\"%s\">%s</a><br />\r\n",
          ep->d_name, ep->d_name);
      count++;
    }
  }
  closedir(dp);
  if (count == 0) {
    mg_printf(conn, "... The directory is empty.\r\n");
  }
  mg_printf(conn, "</BODY>\r\n</HTML>\r\n");
}
