#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef _WIN32
#include <sys/mman.h>
#else
#include <windows.h>
#endif

#include "context.h"
#include "io.h"

static int stash_names(Context *ctx, const char *orig, const char *new, char err[1024])
{
    /* Stash filenames. */
    ctx->oname = malloc(strlen(orig) + 1);
    if (ctx->oname == NULL) {
        strcpy(err, "Could not allocate oname buffer.\n");
        return 1;
    }
    ctx->nname = malloc(strlen(new) + 1);
    if (ctx->nname == NULL) {
        strcpy(err, "Could not allocate oname buffer.\n");
        return 1;
    }

    strcpy(ctx->oname, orig);
    strcpy(ctx->nname, new);

    return 0;
}

static int fill_filesize(Context *ctx, char err[1024])
{
    int ret;
    struct stat st;

    /* Fill in filesize. */
    ret = stat(ctx->oname, &st);
    if (ret != 0) {
        sprintf(err, "Could not stat %s.\n", ctx->oname);
        return 1;
    }
    ctx->osize = st.st_size;

    memset(&st, 0, sizeof(st));

    ret = stat(ctx->nname, &st);
    if (ret != 0) {
        sprintf(err, "Could not stat %s.\n", ctx->nname);
        return 1;
    }
    ctx->nsize = st.st_size;

    return 0;
}

#ifndef _WIN32

/* Standard POSIX memory mapping. */
int open_files(Context *ctx, const char *orig, const char *new, char err[1024])
{
    int nfd = -1;
    int ofd = -1;
    int ret = 0;

    ret = stash_names(ctx, orig, new, err);
    if (ret < 0)
        goto fail;

    ret = fill_filesize(ctx, err);
    if (ret < 0)
        goto fail;

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

    ret = 0;

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

#else

/* Windows memory mapping. */
int open_files(Context *ctx, const char *orig, const char *new, char err[1024])
{
    HANDLE nfd  = INVALID_HANDLE_VALUE;
    HANDLE ofd  = INVALID_HANDLE_VALUE;
    HANDLE mnfd = INVALID_HANDLE_VALUE;
    HANDLE mofd = INVALID_HANDLE_VALUE;
    int ret     = 0;

    ret = stash_names(ctx, orig, new, err);
    if (ret < 0)
        goto fail;

    ret = fill_filesize(ctx, err);
    if (ret < 0)
        goto fail;

    ofd = CreateFile(ctx->oname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (ofd == INVALID_HANDLE_VALUE) {
        sprintf(err, "Could not open %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }

    mofd = CreateFileMapping(ofd, NULL, PAGE_READONLY, 0, 0, 0);
    if (mofd == INVALID_HANDLE_VALUE) {
        sprintf(err, "Could not create mapping for %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }

    ctx->obuf = MapViewOfFile(mofd, FILE_MAP_READ, 0, 0, 0);
    if (ctx->obuf == NULL) {
        sprintf(err, "Could not mmap %s.\n", ctx->oname);
        ret = 1;
        goto fail;
    }

    CloseHandle(ofd);
    CloseHandle(mofd);
    ofd  = INVALID_HANDLE_VALUE;
    mofd = INVALID_HANDLE_VALUE;

    nfd = CreateFile(ctx->nname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (nfd == INVALID_HANDLE_VALUE) {
        sprintf(err, "Could not open %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }

    mnfd = CreateFileMapping(nfd, NULL, PAGE_READONLY, 0, 0, 0);
    if (mnfd == INVALID_HANDLE_VALUE) {
        sprintf(err, "Could not create mapping for %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }

    ctx->nbuf = MapViewOfFile(mnfd, FILE_MAP_READ, 0, 0, 0);
    if (ctx->obuf == NULL) {
        sprintf(err, "Could not mmap %s.\n", ctx->nname);
        ret = 1;
        goto fail;
    }

    CloseHandle(nfd);
    CloseHandle(mnfd);
    nfd  = INVALID_HANDLE_VALUE;
    mnfd = INVALID_HANDLE_VALUE;

    ret = 0;

fail:
    if (nfd != INVALID_HANDLE_VALUE)
        CloseHandle(nfd);

    if (ofd != INVALID_HANDLE_VALUE)
        CloseHandle(ofd);

    if (mnfd != INVALID_HANDLE_VALUE)
        CloseHandle(mnfd);

    if (mofd != INVALID_HANDLE_VALUE)
        CloseHandle(mofd);

    return ret;
}

void close_files(Context *ctx)
{
    if (ctx->nname != NULL)
        free(ctx->nname);

    if (ctx->oname != NULL)
        free(ctx->oname);

    if (ctx->obuf != NULL)
        UnmapViewOfFile(ctx->obuf);

    if (ctx->nbuf != NULL)
        UnmapViewOfFile(ctx->nbuf);
}

#endif
