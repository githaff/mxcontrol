#ifndef __AUX_H
#define __AUX_H


#include <stdio.h>


#define UNUSED(x) ((void)(x))
#define ARR_SIZE(x) (sizeof(x) / (sizeof(x[0])))
#define MIN(x, y) (x > y ? y : x)
#define MAX(x, y) (x > y ? x : y)

#ifdef DEBUG
void dbg(const char *msg, ...);
void dbg_start(const char *msg, ...);
void dbg_next(const char *msg, ...);
void dbg_end(const char *msg, ...);
#else
#define dbg(...) do {} while (0)
#define dbg_start(...) do {} while (0)
#define dbg_next(...) do {} while (0)
#define dbg_end(...) do {} while (0)
#endif

void msgn(const char *msg, ...);
void msg(const char *msg, ...);
void warn(const char *msg, ...);
void err(const char *msg, ...);
void crit(const char *msg, ...);


#endif /* __AUX_H */
