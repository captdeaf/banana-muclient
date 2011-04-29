/* logger.c
 *
 * Rotating log files.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "logger.h"

pthread_mutexattr_t pth_recurse;
static Logger *syslogger;

static int
mkdir_p(char *pathname) {
  char pbuff[200];
  char *p;
  if (mkdir(pathname, 0755)) {
    if (errno == EEXIST) { return 0; }
    if (errno == ENOENT) {
      snprintf(pbuff, 200, "%s", pathname);
      p = strrchr(pbuff, '/');
      if (p) {
        *(p) = '\0';
        mkdir_p(pbuff);
        return mkdir(pathname, 0777);
      }
    }
  }
  return 0;
}

Logger *
logger_new(char *logname) {
  time_t now;
  char timebuff[20];
  char fpath[200], *ptr;
  Logger *logger = malloc(sizeof(Logger));

  logger->logname = strdup(logname);
  // Create the directory if it doesn't exist.
  snprintf(fpath, 200, "%s", logname);
  ptr = strrchr(fpath, '/');
  if (ptr) {
    *(ptr) = '\0';
    mkdir_p(fpath);
  }

  now = time(NULL);
  localtime_r(&now, &logger->logday);

  strftime(timebuff, 20, "%F", &logger->logday);
  logger->datestr = strdup(timebuff);

  pthread_mutex_init(&logger->logmutex, &pth_recurse);
  logger->logout = NULL;

  llog(logger, "-- LOGGING STARTED --");

  return logger;
}

void
logger_free(Logger *logger) {
  llog(logger, "-- LOGGING ENDED --");

  if (logger->logout) {
    fclose(logger->logout);
  }

  free(logger->logname);
  free(logger->datestr);
  pthread_mutex_destroy(&logger->logmutex);
}

void
llog(Logger *logger, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vllog(logger, fmt, args);
  va_end(args);
}

void
vllog(Logger *logger, char *fmt, va_list args) {
  struct tm today;
  time_t now;
  char filename[200];
  char timebuff[20];

  // Lock up.
  pthread_mutex_lock(&logger->logmutex);

  now = time(NULL);
  localtime_r(&now, &today);

  if (today.tm_yday != logger->logday.tm_yday) {
    // Date has advanced.
    localtime_r(&now, &logger->logday);
    free(logger->datestr);
    strftime(timebuff, 20, "%F", &logger->logday);
    logger->datestr = strdup(timebuff);

    if (logger->logout) {
      fclose(logger->logout);
      logger->logout = NULL;
    }
  }

  if (logger->logout == NULL) {
    // Open the log file.
    snprintf(filename, 200, "%s-%s.log", logger->logname, logger->datestr);
    logger->logout = fopen(filename, "a");

    // If it was unable to initialize: Panic.
    if (logger->logout == NULL) {
      if (syslogger && logger != syslogger) {
        pthread_mutex_lock(&syslogger->logmutex);
        slog("Logging error: Unable to open '%s'!\n", filename);
        strerror_r(errno, filename, 100);
        slog("Logging error: %s\n", filename);
        pthread_mutex_unlock(&syslogger->logmutex);
      } else {
        printf("Logging error: Unable to open '%s'!\n", filename);
        strerror_r(errno, filename, 100);
        printf("Logging error: %s\n", filename);
      }
      pthread_mutex_unlock(&logger->logmutex);
      return;
    }
  }

  strftime(timebuff, 20, "%T", &today);

  fprintf(logger->logout, "%s ", timebuff);

  vfprintf(logger->logout, fmt, args);
  fprintf(logger->logout, "\n");
  fflush(logger->logout);

  pthread_mutex_unlock(&logger->logmutex);
}

void
logger_init(char *logpath) {
  pthread_mutexattr_init(&pth_recurse);
  pthread_mutexattr_settype(&pth_recurse, PTHREAD_MUTEX_RECURSIVE);
  syslogger = logger_new(logpath);
}

void
logger_shutdown() {
  logger_free(syslogger);
  syslogger = NULL;
}


static int ttylog = 1;

void
slog(char *fmt, ...) {
  va_list args;
  if (syslogger) {
    pthread_mutex_lock(&syslogger->logmutex);
  }
  va_start(args, fmt);
  vllog(syslogger, fmt, args);
  va_end(args);
  if (ttylog) {
    if (isatty(1) || !syslogger) {
      va_start(args, fmt);
      vprintf(fmt, args);
      va_end(args);
      printf("\n");
    } else {
      printf("Non-tty stdout detected. Logging only to %s",
             syslogger->logname);
      ttylog = 0;
    }
  }
  if (syslogger) {
    pthread_mutex_unlock(&syslogger->logmutex);
  }
}
