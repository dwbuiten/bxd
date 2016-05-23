#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "context.h"
#include "io.h"

int open_files(Context *ctx, const char *orig, const char *new, char err[1024])
{
    int nfd = -1;
    int ofd = -1;
    int ret = 0;
    struct stat st;

    /* Stash filenames. */
    ctx->oname = malloc(strlen(orig) + 1);
    if (ctx->oname == NULL) {
        strcpy(err, "Could not allocate oname buffer.\n");
        ret = 1;
        goto fail;
    }
    ctx->nname = malloc(strlen(new) + 1);
    if (ctx->nname == NULL) {
        strcpy(err, "Could not allocate oname buffer.\n");
        ret = 1;
        goto fail;
    }
    strcpy(ctx->oname, orig);
    strcpy(ctx->nname, new);

    /* Fill in filesize. */
    ret = stat(ctx->oname, &st);
    if (ret != 0) {
        sprintf(err, "Could not stat %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }
    ctx->osize = st.st_size;
    memset(&st, 0, sizeof(st));
    ret = stat(ctx->nname, &st);
    if (ret != 0) {
        sprintf(err, "Could not stat %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }
    ctx->nsize = st.st_size;

    ofd = open(ctx->oname, O_RDONLY, 0);
    if (ofd < 0) {
        sprintf(err, "Could not open %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }
    ctx->obuf = mmap(NULL, ctx->osize, PROT_READ, MAP_PRIVATE, ofd, 0);
    if (ctx->obuf == MAP_FAILED) {
        sprintf(err, "Could not mmap %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }
    close(ofd);
    ofd = -1;

    nfd = open(ctx->nname, O_RDONLY, 0);
    if (nfd < 0) {
        sprintf(err, "Could not open %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }
    ctx->nbuf = mmap(NULL, ctx->nsize, PROT_READ, MAP_PRIVATE, nfd, 0);
    if (ctx->nbuf == MAP_FAILED) {
        sprintf(err, "Could not mmap %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }
    close(nfd);
    nfd = -1;

    return 0;

fail:
    if (ofd > 0)
        close(ofd);

    if (nfd > 0)
        close(nfd);

    return ret;
}

void close_files(Context *ctx)
{
    if (ctx->nname != NULL)
        free(ctx->nname);

    if (ctx->oname != NULL)
        free(ctx->oname);

    if (ctx->obuf != MAP_FAILED && ctx->obuf != NULL) {
        int mun = munmap(ctx->obuf, ctx->osize);
        assert(mun == 0);
    }

    if (ctx->nbuf != MAP_FAILED && ctx->nbuf != NULL) {
        int mun = munmap(ctx->nbuf, ctx->nsize);
        assert(mun == 0);
    }
}
