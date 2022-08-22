#ifndef _IO_H
#define _IO_H

#include "context.h"

int open_files(Context *ctx, const char *orig, const char *new, char err[1024]);
void close_files(Context *ctx);

#endif
