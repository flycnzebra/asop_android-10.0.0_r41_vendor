

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdarg.h>


#include <ctype.h>
#include <err.h>

#include "head.h"



#define ARRAYLEN(a)	(sizeof(a)/sizeof(a[0]))

u_int32_t loglevel = LLV_BASE;

extern int g_log_enable;

char * timeString() {
  struct timespec ts;
  clock_gettime( CLOCK_REALTIME, &ts);
  struct tm * timeinfo = localtime(&ts.tv_sec);
  static char timeStr[20];
  sprintf(timeStr, "%.2d-%.2d %.2d:%.2d:%.2d.%.3ld", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ts.tv_nsec / 1000000);
  return timeStr;
 
}

void do_plog(int level, char *format, ...)
{
    
    if ((g_log_enable||level <= LLV_WARNING)
        &&level >= LLV_ERROR&& level <= loglevel) {        
        static int levels[6] = {
            ANDROID_LOG_ERROR, ANDROID_LOG_WARN, ANDROID_LOG_INFO,
            ANDROID_LOG_INFO, ANDROID_LOG_DEBUG, ANDROID_LOG_VERBOSE
        };
        
        va_list ap;
        va_start(ap, format);
        if(g_log_enable||level <= LLV_WARNING)
            __android_log_vprint(levels[level], "mpcd", format, ap);

        va_end(ap);

    }
    
}



