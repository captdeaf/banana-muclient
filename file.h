/** users.h
 *
 * Defines users for Banana, including their worlds.
 */

#ifndef _MY_FILE_H_
#define _MY_FILE_H_

// Return a file's size. If it doesn't exist, return -1.
int file_size(const char *name);
#define file_exists(x) (file_size(x) >= 0)

// Return vals from this need to be free'd.
char *file_read(const char *name);

// Write to a file. 0 is success, otherwise it's errno.
int file_write(const char *name, const char *contents);
int file_write_len(const char *fname, const char *contents, int len);

// For file-based config stuff.
int file_yorn(const char *fname);
int file_readnum(const char *fname, int def);
int file_writenum(const char *fname, int val);

#endif
