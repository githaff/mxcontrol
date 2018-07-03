#ifndef __AUX_H
#define __AUX_H


#include <stdio.h>


#define ARR_SIZE(x) (sizeof(x) / (sizeof(x[0])))


void msgn(const char *msg, ...);
void msg(const char *msg, ...);
void warn(const char *msg, ...);
void err(const char *msg, ...);
void crit(const char *msg, ...);


#endif /* __AUX_H */
