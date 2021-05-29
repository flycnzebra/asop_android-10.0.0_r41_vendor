
#ifndef _PLOG_H
#define _PLOG_H


#define __FILENAME__ (strrchr(__FILE__, '/')?(strrchr(__FILE__, '/')+1):__FILE__)

#define MPCLOG(logLevel, pFormat, ...)  do_plog(logLevel, "MPC[%lu][%s]line %5d: "pFormat, pthread_self(),__FILENAME__, __LINE__, ##__VA_ARGS__)
#define MAX_LOG_LENGTH 500

#include <stdarg.h>

#include <syslog.h>

#define LLV_ERROR	1
#define LLV_WARNING	2
#define LLV_NOTIFY	3
#define LLV_INFO	4
#define LLV_DEBUG	5
#define LLV_DEBUG2	6

#define LLV_BASE	LLV_DEBUG 

extern void do_plog(int level, char *format, ...);



#endif 
