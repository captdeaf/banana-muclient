/** logger.h
 *
 * Logging-related stuff.
 */

#ifndef _MY_LOGGER_H_
#define _MY_LOGGER_H_

typedef struct logger Logger;

struct logger {
  FILE *logout;

  // The logname path and prefix: "users/foo/log/worldname"
  // This has "-YYYY-MM-DD.log" appended to it, and rotated daily.
  char *logname;

  // YYYY-MM-DD
  char *datestr;

  // Date string.
  struct tm logday;

  // So we don't have overlapping logs.
  pthread_mutex_t logmutex;

  // TODO: Allowing users to rotate logs based on their timezone,
  //       not forcing all to use central time.
};

Logger *logger_new(char *logname);
void logger_free(Logger *logger);

void llog(Logger *logger, char *fmt, ...);
void vllog(Logger *logger, char *fmt, va_list args);

// Log to the log passed by logger_init.
void slog(char *fmt, ...);

// Init the ba system log.
void logger_init(char *logpath);
void logger_shutdown();

#endif /* _API_INC_H_ */
