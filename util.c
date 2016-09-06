#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "lcs.h"
#include "draw.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))

void load_previous(Context *ctx, uint8_t *lbuf, size_t *scratch)
{
    off_t old_pos = ctx->offset_pos - 1;

    ctx->offset = ctx->prev_offset[old_pos];

    ctx->nf_offset -= ctx->prev_nshift[old_pos];
    ctx->of_offset -= ctx->prev_oshift[old_pos];

    ctx->offset_pos--;

    if (!ctx->is_cleared) {
        memset(scratch, 0, (ctx->blocksize + 1) * (ctx->blocksize + 1) * sizeof(*scratch));

        memset(lbuf, 0, ctx->blocksize);

        memset(&ctx->odiff[0], 0, ctx->odsize);
        memset(&ctx->ndiff[0], 0, ctx->ndsize);
    }
    ctx->ndsize = 0;
    ctx->odsize = 0;

    calc_lcs_mask(ctx, lbuf, scratch);
}

bool calc_next_mask(Context *ctx, uint8_t *lbuf, size_t *scratch, bool *err)
{
    off_t old_pos = ctx->offset_pos;
    size_t cmp_size;

    *err = false;

    /* Dummy call. */
    draw_ui_dummy(ctx);

    if ((size_t) old_pos >= ctx->prev_offset_size) {
        uint16_t *new_prev_offset;
        uint16_t *new_prev_oshift;
        uint16_t *new_prev_nshift;

        ctx->prev_offset_size *= 2;

        new_prev_offset = realloc(ctx->prev_offset, ctx->prev_offset_size * sizeof(*new_prev_offset));
        if (new_prev_offset == NULL) {
            *err = true;
            return false;
        }
        ctx->prev_offset = new_prev_offset;

        new_prev_oshift = realloc(ctx->prev_oshift, ctx->prev_offset_size * sizeof(*new_prev_oshift));
        if (new_prev_oshift == NULL) {
            *err = true;
            return false;
        }
        ctx->prev_oshift = new_prev_oshift;

        new_prev_nshift = realloc(ctx->prev_nshift, ctx->prev_offset_size * sizeof(*new_prev_nshift));
        if (new_prev_nshift == NULL) {
            *err = true;
            return false;
        }
        ctx->prev_nshift = new_prev_nshift;
    }

    ctx->prev_offset[old_pos] = ctx->offset - 2;
    ctx->prev_oshift[old_pos] = ctx->oshift;
    ctx->prev_nshift[old_pos] = ctx->nshift;
    ctx->offset_pos++;

    ctx->nf_offset += ctx->nshift;
    ctx->of_offset += ctx->oshift;

    if (!ctx->is_cleared) {
        memset(lbuf, 0, ctx->blocksize);

        memset(scratch, 0, (ctx->blocksize + 1) * (ctx->blocksize + 1) * sizeof(*scratch));
        memset(&ctx->odiff[0], 0, ctx->odsize);
        memset(&ctx->ndiff[0], 0, ctx->ndsize);

        ctx->is_cleared = true;
    }
    ctx->ndsize = 0;
    ctx->odsize = 0;

    ctx->offset = 0;

    cmp_size = MIN(MIN(ctx->osize - ctx->of_offset, ctx->blocksize), ctx->nsize - ctx->nf_offset);

    if (!memcmp(&ctx->obuf[ctx->of_offset], &ctx->nbuf[ctx->nf_offset], cmp_size)) {
        ctx->ndsize = cmp_size;
        ctx->odsize = cmp_size;
        if (cmp_size < ctx->blocksize)
            ctx->done = true;
        return true;
    }

    return calc_lcs_mask(ctx, lbuf, scratch);
}
