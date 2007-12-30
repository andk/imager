#ifndef IMAGER_RENDERT_H
#define IMAGER_RENDERT_H

typedef struct {
  int magic;
  i_img *im;
  i_color *line_8;
  i_fcolor *line_double;
  i_color *fill_line_8;
  i_fcolor *fill_line_double;
  int width;
} i_render;

#endif
