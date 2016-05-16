#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "context.h"
#include "lcs.h"

#define MAX(x,y) (x > y ? x : y)

static size_t lcs (uint8_t *obuf, size_t osize, uint8_t *nbuf, size_t nsize,
                   uint8_t *lbuf, size_t *scratch)
{
    size_t lsize;

    for (size_t i = 1; i <= osize; i++) {
        for (size_t j = 1; j <= nsize; j++) {
            size_t lpos =  i      * (nsize + 1) + j;
            size_t rpos = (i - 1) * (nsize + 1) + j;

            if (obuf[i - 1] == nbuf[j - 1])
                scratch[lpos] = scratch[rpos - 1] + 1;
            else
                scratch[lpos] = MAX(scratch[rpos], scratch[rpos + nsize]);
        }
    }

    lsize = scratch[osize * (nsize + 1) + nsize];

    {
        size_t op = osize;
        size_t np = nsize;

        for (int i = (int) lsize - 1; i >= 0; i--) {
            size_t lpos =  op      * (nsize + 1) + np - 1;
            size_t rpos = (op - 1) * (nsize + 1) + np;

            if (obuf[op - 1] == nbuf[np - 1]) {
                lbuf[i] = obuf[op - 1];
                op--;
                np--;
            } else if (scratch[lpos] > scratch[rpos]) {
                np--;
                i++;
            } else {
                op--;
                i++;
            }
        }
    }

    return lsize;
}

#undef MAX

static void update_masks(Context *ctx, size_t *podp, size_t *pndp, int subrun, int addrun)
{
    uint8_t *odiff = ctx->odiff;
    uint8_t *ndiff = ctx->ndiff;
    size_t odp = *podp;
    size_t ndp = *pndp;

    /* for nbuf */
    for (int j = 0; j < subrun; j++)
        ndiff[ndp + j] = MISSING;
    for (int j = 0; j < addrun; j++) {
        if (ndiff[ndp + j] == MISSING)
            ndiff[ndp + j] = CHANGED;
        else if (ndiff[ndp + j] == SAME)
            ndiff[ndp + j] = CHANGED;
    }
    /* For obuf. */
    for (int j = 0; j < addrun; j++)
        odiff[odp + j] = MISSING;
    for (int j = 0; j < subrun; j++) {
        if (odiff[odp + j] == MISSING)
            odiff[odp + j] = CHANGED;
        else if (odiff[odp + j] == SAME)
            odiff[odp + j] = CHANGED;
    }

    if (subrun < addrun) {
        *pndp += addrun + 1;
        *podp += addrun + 1;
    } else {
        *pndp += subrun + 1;
        *podp += subrun + 1;
    }
}

bool calc_lcs_mask(Context *ctx, uint8_t *lbuf, size_t *scratch)
{
    uint8_t *obuf  = ctx->obuf;
    uint8_t *nbuf  = ctx->nbuf;
    size_t blocksize = ctx->blocksize;

    size_t odp = 0;
    size_t ndp = 0;
    size_t op = 0;
    size_t np = 0;
    int subrun = 0;
    int addrun = 0;

    off_t ooff = ctx->of_offset;
    off_t noff = ctx->nf_offset;
    int ret;

    size_t lsz = lcs(&obuf[ooff], blocksize, &nbuf[noff], blocksize, lbuf, scratch);

    ret = (lsz == blocksize);

    for (size_t i = 0; i < lsz; i++) {
        if (obuf[ooff + op] == lbuf[i] && nbuf[noff + np] == lbuf[i]) {
            if (subrun || addrun) {
                update_masks(ctx, &odp, &ndp, subrun, addrun);
                subrun = 0;
                addrun = 0;
            } else {
                odp++;
                ndp++;
            }
            op++;
            np++;
            continue;
        } else if (obuf[ooff + op] != lbuf[i]) {
            subrun++;
            op++;
            i--;
        } else if (nbuf[noff + np] != lbuf[i]) {
            addrun++;
            np++;
            i--;
        }
    }
    subrun = blocksize - op;
    addrun = blocksize - np;
    update_masks(ctx, &odp, &ndp, subrun, addrun);

    ctx->odsize = odp - 1;
    ctx->ndsize = ndp - 1;

    return ret;
}
