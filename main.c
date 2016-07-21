#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termbox.h>

#include "context.h"
#include "draw.h"
#include "io.h"
#include "lcs.h"
#include "util.h"

#define BLOCKSIZE (1024)

int main(int argc, char *argv[])
{
    uint8_t lbuf[BLOCKSIZE] = { 0 };
    size_t *scratch = NULL;
    int ret = 0;
    int initted = false;
    Context ctx;
    char err[1024];

    if (argc < 3) {
        fprintf(stderr, "Binary Hex Diff\n"
                        "Copyright (c) 2016 Derek Buitenhuis.\n\n"
                        "A tool to compare two arbitrarily sized binary files.\n\n"
                        "Usage: %s file1 file2\n\n", argv[0]);
        return 0;
    }

    memset(&ctx, 0, sizeof(ctx));

    /* Allocations. */
    ctx.odiff = calloc(BLOCKSIZE * BLOCKSIZE, 1);
    if (ctx.odiff == NULL) {
        fprintf(stderr, "Could not allocate odiff buffer.\n");
        ret = 1;
        goto end;
    }
    ctx.ndiff = calloc(BLOCKSIZE * BLOCKSIZE, 1);
    if (ctx.ndiff == NULL) {
        fprintf(stderr, "Could not allocate ndiff buffer.\n");
        ret = 1;
        goto end;
    }
    scratch = calloc((BLOCKSIZE + 1) * (BLOCKSIZE + 1), sizeof(*scratch));
    if (scratch == NULL) {
        fprintf(stderr, "Could not allocate LCS scratch buffer.\n");
        ret = 1;
        goto end;
    }

    ctx.prev_offset = malloc(1024 * 1024 * sizeof(*(ctx.prev_offset)));
    ctx.prev_nshift = malloc(1024 * 1024 * sizeof(*(ctx.prev_nshift)));
    ctx.prev_oshift = malloc(1024 * 1024 * sizeof(*(ctx.prev_oshift)));
    if (ctx.prev_offset == NULL || ctx.prev_oshift == NULL || ctx.prev_nshift == NULL) {
        fprintf(stderr, "Could not allocate offset tracking buffers.\n");
        ret = 1;
        goto end;
    }
    ctx.prev_offset_size = 1024 * 1024;

    ctx.blocksize = BLOCKSIZE;

    ret = open_files(&ctx, argv[1], argv[2], err);
    if (ret != 0 ) {
        fputs(err, stderr);
        ret = 1;
        goto end;
    }

    calc_lcs_mask(&ctx, &lbuf[0], scratch);

    ret = tb_init();
    if (ret != 0) {
        fprintf(stderr, "Init failed: %d.\n", ret);
        ret = 1;
        goto end;
    }
    initted = true;

    draw_ui(&ctx);
    tb_present();

    {
        struct tb_event ev;

        while (tb_poll_event(&ev)) {
            switch (ev.type) {
            case TB_EVENT_RESIZE:
                draw_ui(&ctx);
                break;
            case TB_EVENT_KEY:
                switch (ev.key) {
                case TB_KEY_ARROW_UP:
                    if (ctx.offset == 0) {
                        if (ctx.nf_offset != 0 && ctx.of_offset != 0)
                            load_previous(&ctx, &lbuf[0], scratch);
                    } else {
                        ctx.offset -= 2;
                    }
                    break;
                case TB_KEY_ARROW_DOWN: {
                    unsigned int cpl = get_char_per_line();
                    unsigned int lpu = get_line_per_side();

                    if (ctx.done)
                        continue;

                    ctx.offset += 2;

                    if (((size_t) ctx.offset / 2) * cpl + cpl * lpu >= ctx.blocksize) {
                        bool err;

                        calc_next_mask(&ctx, &lbuf[0], scratch, &err);

                        if (err)
                            goto end;
                    }

                    break;
                }
                case TB_KEY_SPACE: {
                    unsigned int cpl = get_char_per_line();
                    unsigned int lpu = get_line_per_side();
                    bool err = false;

                    do {
                        if (err)
                            goto end;

                        while (((size_t) ctx.offset / 2) * cpl + cpl * lpu < ctx.blocksize)
                            ctx.offset += 2;
                    } while(calc_next_mask(&ctx, &lbuf[0], scratch, &err) && !ctx.done);

                    if (err)
                        goto end;

                    break;
                }
                case 0:
                    if (ev.ch == 'q')
                        goto end;
                    continue;
                default:
                    continue;
                }
                draw_ui(&ctx);
            }
            tb_present();
        }
    }

end:
    if (ctx.ndiff != NULL)
        free(ctx.ndiff);

    if (ctx.odiff != NULL)
        free(ctx.odiff);

    if (scratch != NULL)
        free(scratch);

    if (ctx.prev_offset != NULL)
        free(ctx.prev_offset);

    if (ctx.prev_oshift != NULL)
        free(ctx.prev_oshift);

    if (ctx.prev_nshift != NULL)
        free(ctx.prev_nshift);

    close_files(&ctx);

    if (initted)
        tb_shutdown();

    return ret;
}
