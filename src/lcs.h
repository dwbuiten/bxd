#ifndef _LCS_H
#define _LCS_H

#include <stdbool.h>
#include <stdint.h>

#include "context.h"

/* Change types. */
#define SAME 0
#define CHANGED 1
#define MISSING 2

bool calc_lcs_mask(Context *ctx, uint8_t *lbuf, size_t *scratch);

#endif
