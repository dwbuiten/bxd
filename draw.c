#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <termbox.h>

#include "context.h"
#include "lcs.h"

unsigned int get_char_per_line(void)
{
    int w = tb_width();

    return (w / 2 - 4) / 4 - 1;
}

unsigned int get_line_per_side(void)
{
    int h = tb_height();
    unsigned int ret = 0;

    for (int i = 3; i < h - 1; i += 2)
        ret++;

    return ret;
}

static inline char get_char(char c)
{
    if (c > 32 && c < 127)
        return c;
    else
        return '.';
}

#define FBUFOK (np + (size_t) ctx->nf_offset < ctx->nsize || op + (size_t) ctx->of_offset < ctx->osize)
#define BUFOK (FBUFOK && odp < ctx->odsize && ndp < ctx->ndsize)

void draw_ui_dummy(Context *ctx)
{
    int w;
    int h;
    int ratio;
    unsigned int op  = 0;
    unsigned int np  = 0;
    unsigned int odp = 0;
    unsigned int ndp = 0;

    w     = tb_width();
    h     = tb_height();
    ratio = ((w / 2 - 4) / 4) * 3;

    for (int j = 3; j < h - 1 + ctx->offset; j += 2) {
        int should_draw = (j - 3 >= ctx->offset);

        j -= ctx->offset;

        for (int i = 3; i < ratio && BUFOK; i += 3) {
            switch (ctx->ndiff[ndp]) {
            case CHANGED:
            case SAME:
                np++;

                if (!should_draw)
                    ctx->nshift = np;

                break;
            case MISSING:
                break;
            }
            ndp++;

            switch (ctx->odiff[odp]) {
            case CHANGED:
            case SAME:
                op++;

                if (!should_draw)
                    ctx->oshift = op;

                break;
            case MISSING:
                break;
            }
            odp++;
        }

        j += ctx->offset;
    }

    ctx->done = !FBUFOK;
}

void draw_ui(Context *ctx)
{
    struct tb_cell *buf;
    int w;
    int h;
    int ratio;
    unsigned int op  = 0;
    unsigned int np  = 0;
    unsigned int odp = 0;
    unsigned int ndp = 0;
    off_t ooff = ctx->of_offset;
    off_t noff = ctx->nf_offset;

    tb_clear();

    buf   = tb_cell_buffer();
    w     = tb_width();
    h     = tb_height();
    ratio = ((w / 2 - 4) / 4) * 3;

    /* Corners. */
    buf[0    ].ch = 0x2554;
    buf[w - 1].ch = 0x2557;
    buf[w * (h - 1)        ].ch = 0x255A;
    buf[w * (h - 1) + w - 1].ch = 0x255D;

    /* Sides. */
    for (int i = 1; i < w - 1; i++) {
        buf[i].ch = 0x2550;
        buf[w * (h - 1) + i].ch = 0x2550;
    }

    /* Top / Bottom. */
    for (int i = 1; i < h - 1; i++) {
        buf[w * i        ].ch = 0x2551;
        buf[w * i + w - 1].ch = 0x2551;
    }

    /* Middle. */
    buf[w / 2              ].ch = 0x2564;
    buf[w * (h - 1) + w / 2].ch = 0x2567;
    for (int i = 1; i < h - 1; i++)
        buf[w * i + w / 2].ch = 0x2502;

    /* Labels. */
    {
        size_t olen = strlen(ctx->oname);
        size_t nlen = strlen(ctx->nname);
        int p1 = (w / 4);
        int p2 = (w / 4) * 3;

        if (p1 - (olen / 2) > 1) {
            p1 -= olen / 2;
        } else {
            p1 = 1;
            olen = w / 2 - 2;
        }
        if (p2 - (nlen / 2) > ((unsigned int) w / 2) + 1) {
            p2 -= nlen / 2;
        } else {
            p2   = (w / 2 ) + 1;
            nlen = w / 2 - 2;
        }

        for (size_t i = 0; i < olen; i++) {
            buf[w + p1 + i].fg |= TB_UNDERLINE;
            buf[w + p1 + i].ch  = ctx->oname[i];
        }

        for (size_t i = 0; i < nlen; i++) {
            buf[w + p2 + i].fg |= TB_UNDERLINE;
            buf[w + p2 + i].ch  = ctx->nname[i];
        }
    }

    buf[w + 3].ch = 'O';
    buf[w + 4].ch = 'f';
    buf[w + 5].ch = 'f';
    buf[w + 6].ch = 's';
    buf[w + 7].ch = 'e';
    buf[w + 8].ch = 't';
    buf[w + 9].ch = ':';

    buf[w + w / 2 + 3].ch = 'O';
    buf[w + w / 2 + 4].ch = 'f';
    buf[w + w / 2 + 5].ch = 'f';
    buf[w + w / 2 + 6].ch = 's';
    buf[w + w / 2 + 7].ch = 'e';
    buf[w + w / 2 + 8].ch = 't';
    buf[w + w / 2 + 9].ch = ':';

    for (int i = 3; i <= 9; i++) {
        buf[w         + i].fg |= TB_BOLD;
        buf[w + w / 2 + i].fg |= TB_BOLD;
    }

    ctx->oshift = 0;
    ctx->nshift = 0;

    for (int j = 3; j < h - 1 + ctx->offset; j += 2) {
        int nprev = 0;
        int oprev = 0;
        int cnt = 3;
        int should_draw = (j - 3 >= ctx->offset);

        j -= ctx->offset;

        for (int i = 3; i < ratio && BUFOK; i += 3) {
            char sbuf[3];

            switch (ctx->ndiff[ndp]) {
            case CHANGED:
                if (should_draw) {
                    if (nprev)
                        buf[w * j + w / 2 + i - 1].bg = TB_WHITE;

                    buf[w * j + w / 2 + i    ].bg = TB_WHITE;
                    buf[w * j + w / 2 + i + 1].bg = TB_WHITE;
                    buf[w * j + w / 2 + i    ].fg = TB_BLACK | TB_BOLD;
                    buf[w * j + w / 2 + i + 1].fg = TB_BLACK | TB_BOLD;
 
                    buf[w * j + w / 2 + ratio + cnt].bg = TB_WHITE;
                    buf[w * j + w / 2 + ratio + cnt].fg = TB_BLACK | TB_BOLD;
                }
            case SAME:
                if (should_draw) {
                    buf[w * j + w / 2 + ratio + cnt].ch = get_char(ctx->nbuf[np + noff]);

                    sprintf(sbuf, "%02X", ctx->nbuf[np + noff]);
                    buf[w * j + w / 2 + i    ].ch = sbuf[0];
                    buf[w * j + w / 2 + i + 1].ch = sbuf[1];
                }
                np++;

                if (!should_draw)
                    ctx->nshift = np;

                if (should_draw)
                    nprev = (ctx->ndiff[ndp] == CHANGED);

                break;
            case MISSING:
                if (should_draw) {
                    if (nprev)
                         buf[w * j + w / 2 + i - 1].bg = TB_WHITE;

                    buf[w * j + w / 2 + i    ].bg = TB_WHITE;
                    buf[w * j + w / 2 + i + 1].bg = TB_WHITE;
                    nprev = 1;

                    buf[w * j + w / 2 + ratio + cnt].bg = TB_WHITE;
                }
                break;
            }
            ndp++;

            switch (ctx->odiff[odp]) {
            case CHANGED:
                if (should_draw) {
                    if (oprev)
                         buf[w * j + i - 1].bg = TB_WHITE;

                    buf[w * j + i    ].bg = TB_WHITE;
                    buf[w * j + i + 1].bg = TB_WHITE;
                    buf[w * j + i    ].fg = TB_BLACK | TB_BOLD;
                    buf[w * j + i + 1].fg = TB_BLACK | TB_BOLD;

                    buf[w * j + ratio + cnt].bg = TB_WHITE;
                    buf[w * j + ratio + cnt].fg = TB_BLACK | TB_BOLD;
                }
            case SAME:
                if (should_draw) {
                    buf[w * j + ratio + cnt].ch = get_char(ctx->obuf[op + ooff]);

                    sprintf(sbuf, "%02X", ctx->obuf[op + ooff]);
                    buf[w * j + i    ].ch = sbuf[0];
                    buf[w * j + i + 1].ch = sbuf[1];
                }
                op++;

                if (!should_draw)
                    ctx->oshift = op;

                if(should_draw)
                    oprev = (ctx->odiff[odp] == CHANGED);

                break;
            case MISSING:
                if(should_draw) {
                    if (oprev)
                         buf[w * j + i - 1].bg = TB_WHITE;

                    buf[w * j + i    ].bg = TB_WHITE;
                    buf[w * j + i + 1].bg = TB_WHITE;
                    oprev = 1;

                    buf[w * j + ratio + cnt].bg = TB_WHITE;
                }
                break;
            }
            odp++;

            cnt++;
        }

        j += ctx->offset;
    }

     ctx->done = !FBUFOK;

    {
        off_t ooff = ctx->of_offset + ctx->oshift;
        off_t noff = ctx->nf_offset + ctx->nshift;
        char osbuf[11];
        char nsbuf[11];

        sprintf(osbuf, "0x%08zX", ooff);
        sprintf(nsbuf, "0x%08zX", noff);
        for (int i = 0; i < 11; i++) {
            buf[w         + 11 + i].ch = osbuf[i];
            buf[w + w / 2 + 11 + i].ch = nsbuf[i];
        }
    }
}
