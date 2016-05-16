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

#define FILE1 "old.nut"
#define FILE2 "new.nut"

int main(int argc, char *argv[])
{
    uint8_t lbuf[BLOCKSIZE] = { 0 };
    size_t *scratch = NULL;
    int ret = 0;
    int initted = false;
    Context ctx = { 0 };
    char err[1024];

    if (argc < 3) {
        fprintf(stderr, "Binary Hex Diff\n"
                        "Copyright (c) 2016 Derek Buitenhuis.\n\n"
                        "A tool to compare two arbitrarily sized binary files.\n\n"
                        "Usage: %s file1 file2\n\n", argv[0]);
        return 0;
    }

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

    ctx.old_of_offset = -1;
    ctx.old_nf_offset = -1;
    ctx.blocksize = BLOCKSIZE;

    ret = open_files(&ctx, argv[1], argv[2], err);
    if (ret != 0 ) {
        fprintf(stderr, err);
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
                        if (ctx.old_nf_offset >= 0 && ctx.old_of_offset >= 0)
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

                    if (((size_t) ctx.offset / 2) * cpl + cpl * lpu >= ctx.blocksize)
                        calc_next_mask(&ctx, &lbuf[0], scratch);

                    break;
                }
                case TB_KEY_SPACE: {
                    unsigned int cpl = get_char_per_line();
                    unsigned int lpu = get_line_per_side();

                    do {
                        while (((size_t) ctx.offset / 2) * cpl + cpl * lpu < ctx.blocksize)
                            ctx.offset += 2;
                    } while((!ctx.done) && calc_next_mask(&ctx, &lbuf[0], scratch));

                    if (ctx.old_offset > 0)
                        ctx.old_offset -= 2;

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

    close_files(&ctx);

    if (initted)
        tb_shutdown();

    return ret;
}
