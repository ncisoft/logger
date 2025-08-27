#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "logger.h"

/**
 * global logger context
 */
static logger_ctx_t g_logger = {
    .fp = NULL,
#ifdef DEBUG
    .level = LOGGER_LEVEL_DEBUG,
#else
    .level = LOGGER_LEVEL_INFO,
#endif
    .fp_level = LOGGER_LEVEL_TRACE,
    .fd = 0,



    .with_color = LOGGER_COLOR_ON
};

logger_ctx_t *logger_get_ctx()
{
    return &g_logger;
}
#define LOGGER_TIMESTAMP_LEN 20
static char *format_time(char *buf) {
    time_t now = time(NULL);

    struct tm tm;
    localtime_r(&now, &tm);

    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    return buf;
}

int logger_init(char *filename, uint8_t options) {
    g_logger.fp = NULL;
    g_logger.with_color = LOGGER_COLOR_OFF;

    if (filename) {
        g_logger.fp = fopen(filename, "a");
        if (NULL == g_logger.fp) {
            return -1;
        }
    }

    if (1 == isatty(STDOUT_FILENO) && LOGGER_COLOR_ON == (options & LOGGER_COLOR_MASK)) {
        g_logger.with_color = LOGGER_COLOR_ON;
    }

    g_logger.level = options & LOGGER_LEVEL_MASK;

    return 0;
}

void logger_close() {
    if (NULL != g_logger.fp) {
        fclose(g_logger.fp);
        g_logger.fp = NULL;
    }
    if (g_logger.fd > 0) {
        close(g_logger.fd);
        g_logger.fd = 0;
    }
}

const char * shorten_filename(const char *fname) {
  char delimiter = '/';
  const char *cp_head = fname;
  char *cp = strrchr(cp_head, delimiter);
  if (cp != NULL) {
    cp --;
    while (cp > cp_head && *cp != delimiter) {
      cp--;
    }
    cp++;
    return cp;
  }
  return fname;
}


int logger_printf(uint8_t log_level, const char *color, const char *format, ...) {
    char timestamp[LOGGER_TIMESTAMP_LEN];
    int nwritten = 0;
    va_list arg;

    if (log_level >= g_logger.fp_level) {

        if (g_logger.fd > 0) {
            int fd = g_logger.fd;
            format_time(timestamp);

            nwritten += write(fd, timestamp, strlen(timestamp));
            nwritten += write(fd, " ", strlen(" "));
            
            va_start(arg, format);
            nwritten += vdprintf(fd, format, arg);
            va_end(arg);
            return nwritten;
        }
    }

    if (log_level < g_logger.level) {
        return 0;
    }

    if (LOGGER_COLOR_ON == g_logger.with_color) {
        fprintf(stdout, "%s", color);
    }

    format_time(timestamp);

    fprintf(stdout, "%s ", timestamp);
    va_start(arg, format);
    nwritten += vfprintf(stdout, format, arg);
    va_end(arg);

    if (LOGGER_COLOR_ON == g_logger.with_color) {
        fprintf(stdout, LOGGER_COLOR_RESET);
        fflush(stdout);
    }

    return nwritten;
}
