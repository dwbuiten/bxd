#ifndef _DRAW_H
#define _DRAW_H

#include "context.h"

unsigned int get_char_per_line(void);
unsigned int get_line_per_side(void);
void draw_ui_dummy(Context *ctx);
void draw_ui(Context *ctx);

#endif
