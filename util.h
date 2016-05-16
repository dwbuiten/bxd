#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "context.h"

void load_previous(Context *ctx, uint8_t *lbuf, size_t *scratch);
bool calc_next_mask(Context *ctx, uint8_t *lbuf, size_t *scratch);

#endif
