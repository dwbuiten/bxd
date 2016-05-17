#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct Context {
    uint8_t *obuf;
    uint8_t *nbuf;
    char *oname;
    char *nname;
    size_t osize;
    size_t nsize;

    off_t of_offset;
    off_t nf_offset;

    uint8_t *odiff;
    uint8_t *ndiff;
    size_t odsize;
    size_t ndsize;
    size_t blocksize;

    size_t nshift;
    size_t oshift;
    off_t offset;

    uint16_t *prev_offset;
    uint16_t *prev_oshift;
    uint16_t *prev_nshift;
    size_t prev_offset_size;
    off_t offset_pos;

    bool done;
} Context;

#endif
