#include <stdarg.h>
#include <stdlib.h>

#include "aux.h"


static void _msgn(FILE *stream, const char *msg, va_list args)
{
	vfprintf(stream, msg, args);
	fflush(stream);
}

void msgn(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	_msgn(stdout, msg, args);
	va_end(args);
}

static void _msg(FILE *stream, const char *msg, va_list args)
{
	_msgn(stream, msg, args);
	fprintf(stream, "\n");
}

void msg(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	_msg(stdout, msg, args);
	va_end(args);
}

void warn(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stderr, "Warning: ");
    _msg(stderr, msg, args);
    va_end(args);
}

void err(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
  fprintf(stderr, "Error: ");
	_msg(stderr, msg, args);
	va_end(args);
}

void crit(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
  fprintf(stderr, "Critical: ");
	err(msg, args);
	va_end(args);

	exit(1);
}
