#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "context.h"
#include "lcs.h"
#include "draw.h"

void load_previous(Context *ctx, uint8_t *lbuf, size_t *scratch)
{
    ctx->nf_offset = ctx->old_nf_offset;
    ctx->of_offset = ctx->old_of_offset;

    memset(lbuf, 0, ctx->blocksize);

    memset(&ctx->odiff[0], 0, ctx->odsize);
    memset(&ctx->ndiff[0], 0, ctx->ndsize);
    ctx->ndsize = 0;
    ctx->odsize = 0;

    ctx->offset = ctx->old_offset;

    ctx->old_nf_offset = -1;
    ctx->old_of_offset = -1;

    calc_lcs_mask(ctx, lbuf, scratch);
}

bool calc_next_mask(Context *ctx, uint8_t *lbuf, size_t *scratch)
{
    /* Dummy call. */
    draw_ui(ctx);

    ctx->old_offset = ctx->offset;
    ctx->old_nf_offset = ctx->nf_offset;
    ctx->old_of_offset = ctx->of_offset;

    ctx->nf_offset += ctx->nshift;
    ctx->of_offset += ctx->oshift;

    memset(lbuf, 0, ctx->blocksize);

    memset(&ctx->odiff[0], 0, ctx->odsize);
    memset(&ctx->ndiff[0], 0, ctx->ndsize);
    ctx->ndsize = 0;
    ctx->odsize = 0;

    ctx->offset = 0;

    if (!memcmp(&ctx->obuf[ctx->of_offset], &ctx->nbuf[ctx->nf_offset], ctx->blocksize)) {
        ctx->ndsize = ctx->blocksize;
        ctx->odsize = ctx->blocksize;
        return true;
    }

    return calc_lcs_mask(ctx, lbuf, scratch);
}
