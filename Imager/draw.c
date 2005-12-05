#include "image.h"
#include "draw.h"
#include "log.h"
#include "imagei.h"

#include <limits.h>

void
i_mmarray_cr(i_mmarray *ar,int l) {
  int i;
  int alloc_size;

  ar->lines=l;
  alloc_size = sizeof(minmax) * l;
  /* check for overflow */
  if (alloc_size / l != sizeof(minmax)) {
    fprintf(stderr, "overflow calculating memory allocation");
    exit(3);
  }
  ar->data=mymalloc(alloc_size); /* checked 5jul05 tonyc */
  for(i=0;i<l;i++) { ar->data[i].max=-1; ar->data[i].min=MAXINT; }
}

void
i_mmarray_dst(i_mmarray *ar) {
  ar->lines=0;
  if (ar->data != NULL) { myfree(ar->data); ar->data=NULL; }
}

void
i_mmarray_add(i_mmarray *ar,int x,int y) {
  if (y>-1 && y<ar->lines)
    {
      if (x<ar->data[y].min) ar->data[y].min=x;
      if (x>ar->data[y].max) ar->data[y].max=x;
    }
}

int
i_mmarray_gmin(i_mmarray *ar,int y) {
  if (y>-1 && y<ar->lines) return ar->data[y].min;
  else return -1;
}

int
i_mmarray_getm(i_mmarray *ar,int y) {
  if (y>-1 && y<ar->lines) return ar->data[y].max;
  else return MAXINT;
}

void
i_mmarray_render(i_img *im,i_mmarray *ar,i_color *val) {
  int i,x;
  for(i=0;i<ar->lines;i++) if (ar->data[i].max!=-1) for(x=ar->data[i].min;x<ar->data[i].max;x++) i_ppix(im,x,i,val);
}

void
i_mmarray_render_fill(i_img *im,i_mmarray *ar,i_fill_t *fill) {
  int x, w, y;
  if (im->bits == i_8_bits && fill->fill_with_color) {
    i_color *line = mymalloc(sizeof(i_color) * im->xsize); /* checked 5jul05 tonyc */
    i_color *work = NULL;
    if (fill->combine)
      work = mymalloc(sizeof(i_color) * im->xsize); /* checked 5jul05 tonyc */
    for(y=0;y<ar->lines;y++) {
      if (ar->data[y].max!=-1) {
        x = ar->data[y].min;
        w = ar->data[y].max-ar->data[y].min;

        if (fill->combine) {
          i_glin(im, x, x+w, y, line);
          (fill->fill_with_color)(fill, x, y, w, im->channels, work);
          (fill->combine)(line, work, im->channels, w);
        }
        else {
          (fill->fill_with_color)(fill, x, y, w, im->channels, line);
        }
        i_plin(im, x, x+w, y, line);
      }
    }
  
    myfree(line);
    if (work)
      myfree(work);
  }
  else {
    i_fcolor *line = mymalloc(sizeof(i_fcolor) * im->xsize); /* checked 5jul05 tonyc */
    i_fcolor *work = NULL;
    if (fill->combinef)
      work = mymalloc(sizeof(i_fcolor) * im->xsize); /* checked 5jul05 tonyc */
    for(y=0;y<ar->lines;y++) {
      if (ar->data[y].max!=-1) {
        x = ar->data[y].min;
        w = ar->data[y].max-ar->data[y].min;

        if (fill->combinef) {
          i_glinf(im, x, x+w, y, line);
          (fill->fill_with_fcolor)(fill, x, y, w, im->channels, work);
          (fill->combinef)(line, work, im->channels, w);
        }
        else {
          (fill->fill_with_fcolor)(fill, x, y, w, im->channels, line);
        }
        i_plinf(im, x, x+w, y, line);
      }
    }
  
    myfree(line);
    if (work)
      myfree(work);
  }
}


static
void
i_arcdraw(int x1, int y1, int x2, int y2, i_mmarray *ar) {
  double alpha;
  double dsec;
  int temp;
  alpha=(double)(y2-y1)/(double)(x2-x1);
  if (fabs(alpha) <= 1) 
    {
      if (x2<x1) { temp=x1; x1=x2; x2=temp; temp=y1; y1=y2; y2=temp; }
      dsec=y1;
      while(x1<=x2)
	{
	  i_mmarray_add(ar,x1,(int)(dsec+0.5));
	  dsec+=alpha;
	  x1++;
	}
    }
  else
    {
      alpha=1/alpha;
      if (y2<y1) { temp=x1; x1=x2; x2=temp; temp=y1; y1=y2; y2=temp; }
      dsec=x1;
      while(y1<=y2)
	{
	  i_mmarray_add(ar,(int)(dsec+0.5),y1);
	  dsec+=alpha;
	  y1++;
	}
    }
}

void
i_mmarray_info(i_mmarray *ar) {
  int i;
  for(i=0;i<ar->lines;i++)
  if (ar->data[i].max!=-1) printf("line %d: min=%d, max=%d.\n",i,ar->data[i].min,ar->data[i].max);
}

static void
i_arc_minmax(i_int_hlines *hlines,int x,int y,float rad,float d1,float d2) {
  i_mmarray dot;
  float f,fx,fy;
  int x1,y1;

  /*mm_log((1,"i_arc(im* 0x%x,x %d,y %d,rad %.2f,d1 %.2f,d2 %.2f,val 0x%x)\n",im,x,y,rad,d1,d2,val));*/

  i_mmarray_cr(&dot, hlines->limit_y);

  x1=(int)(x+0.5+rad*cos(d1*PI/180.0));
  y1=(int)(y+0.5+rad*sin(d1*PI/180.0));
  fx=(float)x1; fy=(float)y1;

  /*  printf("x1: %d.\ny1: %d.\n",x1,y1); */
  i_arcdraw(x, y, x1, y1, &dot);

  x1=(int)(x+0.5+rad*cos(d2*PI/180.0));
  y1=(int)(y+0.5+rad*sin(d2*PI/180.0));

  for(f=d1;f<=d2;f+=0.01) i_mmarray_add(&dot,(int)(x+0.5+rad*cos(f*PI/180.0)),(int)(y+0.5+rad*sin(f*PI/180.0)));
  
  /*  printf("x1: %d.\ny1: %d.\n",x1,y1); */
  i_arcdraw(x, y, x1, y1, &dot);

  /* render the minmax values onto the hlines */
  for (y = 0; y < dot.lines; y++) {
    if (dot.data[y].max!=-1) {
      int minx, width;
      minx = dot.data[y].min;
      width = dot.data[y].max - dot.data[y].min + 1;
      i_int_hlines_add(hlines, y, minx, width);
    }
  }

  /*  dot.info(); */
  i_mmarray_dst(&dot);
}

static void
i_arc_hlines(i_int_hlines *hlines,int x,int y,float rad,float d1,float d2) {
  if (d1 <= d2) {
    i_arc_minmax(hlines, x, y, rad, d1, d2);
  }
  else {
    i_arc_minmax(hlines, x, y, rad, d1, 360);
    i_arc_minmax(hlines, x, y, rad, 0, d2);
  }
}

void
i_arc(i_img *im,int x,int y,float rad,float d1,float d2,i_color *val) {
  i_int_hlines hlines;

  i_int_init_hlines_img(&hlines, im);

  i_arc_hlines(&hlines, x, y, rad, d1, d2);

  i_int_hlines_fill_color(im, &hlines, val);

  i_int_hlines_destroy(&hlines);
}

#define MIN_CIRCLE_STEPS 8
#define MAX_CIRCLE_STEPS 360

void
i_arc_cfill(i_img *im,int x,int y,float rad,float d1,float d2,i_fill_t *fill) {
  i_int_hlines hlines;

  i_int_init_hlines_img(&hlines, im);

  i_arc_hlines(&hlines, x, y, rad, d1, d2);

  i_int_hlines_fill_fill(im, &hlines, fill);

  i_int_hlines_destroy(&hlines);
}

static void
arc_poly(int *count, double **xvals, double **yvals,
	 double x, double y, double rad, double d1, double d2) {
  double d1_rad, d2_rad;
  double circum;
  int steps, point_count;
  double angle_inc;

  /* normalize the angles */
  d1 = fmod(d1, 360);
  if (d1 == 0) {
    if (d2 >= 360) { /* default is 361 */
      d2 = 360;
    }
    else {
      d2 = fmod(d2, 360);
      if (d2 < d1)
	d2 += 360;
    }
  }
  else {
    d2 = fmod(d2, 360);
    if (d2 < d1)
      d2 += 360;
  }
  d1_rad = d1 * PI / 180;
  d2_rad = d2 * PI / 180;

  /* how many segments for the curved part? 
     we do a maximum of one per degree, with a minimum of 8/circle
     we try to aim at having about one segment per 2 pixels
     Work it out per circle to get a step size.

     I was originally making steps = circum/2 but that looked horrible.

     I think there might be an issue in the polygon filler.
  */
  circum = 2 * PI * rad;
  steps = circum;
  if (steps > MAX_CIRCLE_STEPS)
    steps = MAX_CIRCLE_STEPS;
  else if (steps < MIN_CIRCLE_STEPS)
    steps = MIN_CIRCLE_STEPS;

  angle_inc = 2 * PI / steps;

  point_count = steps + 5; /* rough */
  *xvals = mymalloc(point_count * sizeof(double));
  *yvals = mymalloc(point_count * sizeof(double));

  /* from centre to edge at d1 */
  (*xvals)[0] = x;
  (*yvals)[0] = y;
  (*xvals)[1] = x + rad * cos(d1_rad);
  (*yvals)[1] = y + rad * sin(d1_rad);
  *count = 2;

  /* step around the curve */
  while (d1_rad < d2_rad) {
    (*xvals)[*count] = x + rad * cos(d1_rad);
    (*yvals)[*count] = y + rad * sin(d1_rad);
    ++*count;
    d1_rad += angle_inc;
  }

  /* finish off the curve */
  (*xvals)[*count] = x + rad * cos(d2_rad);
  (*yvals)[*count] = y + rad * sin(d2_rad);
  ++*count;
}

void
i_arc_aa(i_img *im, double x, double y, double rad, double d1, double d2,
	 i_color *val) {
  double *xvals, *yvals;
  int count;

  arc_poly(&count, &xvals, &yvals, x, y, rad, d1, d2);

  i_poly_aa(im, count, xvals, yvals, val);

  myfree(xvals);
  myfree(yvals);
}

void
i_arc_aa_cfill(i_img *im, double x, double y, double rad, double d1, double d2,
	       i_fill_t *fill) {
  double *xvals, *yvals;
  int count;

  arc_poly(&count, &xvals, &yvals, x, y, rad, d1, d2);

  i_poly_aa_cfill(im, count, xvals, yvals, fill);

  myfree(xvals);
  myfree(yvals);
}

/* Temporary AA HACK */


typedef int frac;
static  frac float_to_frac(float x) { return (frac)(0.5+x*16.0); }
static   int frac_sub     (frac x)  { return (x%16); }
static   int frac_int     (frac x)  { return (x/16); }
static float frac_to_float(float x) { return (float)x/16.0; }

static 
void
polar_to_plane(float cx, float cy, float angle, float radius, frac *x, frac *y) {
  *x = float_to_frac(cx+radius*cos(angle));
  *y = float_to_frac(cy+radius*sin(angle));
}

static
void
order_pair(frac *x, frac *y) {
  frac t = *x;
  if (t>*y) {
    *x = *y;
    *y = t;
  }
}




static
void
make_minmax_list(i_mmarray *dot, float x, float y, float radius) {
  float angle = 0.0;
  float astep = radius>0.1 ? .5/radius : 10;
  frac cx, cy, lx, ly, sx, sy;

  mm_log((1, "make_minmax_list(dot %p, x %.2f, y %.2f, radius %.2f)\n", dot, x, y, radius));

  polar_to_plane(x, y, angle, radius, &sx, &sy);
  
  for(angle = 0.0; angle<361; angle +=astep) {
    lx = sx; ly = sy;
    polar_to_plane(x, y, angle, radius, &cx, &cy);
    sx = cx; sy = cy;

    if (fabs(cx-lx) > fabs(cy-ly)) {
      int ccx, ccy;
      if (lx>cx) { 
	ccx = lx; lx = cx; cx = ccx; 
	ccy = ly; ly = cy; cy = ccy; 
      }

      for(ccx=lx; ccx<=cx; ccx++) {
	ccy = ly + ((cy-ly)*(ccx-lx))/(cx-lx);
	i_mmarray_add(dot, ccx, ccy);
      }
    } else {
      int ccx, ccy;

      if (ly>cy) { 
	ccy = ly; ly = cy; cy = ccy; 
	ccx = lx; lx = cx; cx = ccx; 
      }
      
      for(ccy=ly; ccy<=cy; ccy++) {
	if (cy-ly) ccx = lx + ((cx-lx)*(ccy-ly))/(cy-ly); else ccx = lx;
	i_mmarray_add(dot, ccx, ccy);
      }
    }
  }
}

/* Get the number of subpixels covered */

static
int
i_pixel_coverage(i_mmarray *dot, int x, int y) {
  frac minx = x*16;
  frac maxx = minx+15;
  frac cy;
  int cnt = 0;
  
  for(cy=y*16; cy<(y+1)*16; cy++) {
    frac tmin = dot->data[cy].min;
    frac tmax = dot->data[cy].max;

    if (tmax == -1 || tmin > maxx || tmax < minx) continue;
    
    if (tmin < minx) tmin = minx;
    if (tmax > maxx) tmax = maxx;
    
    cnt+=1+tmax-tmin;
  }
  return cnt;
}

void
i_circle_aa(i_img *im, float x, float y, float rad, i_color *val) {
  i_mmarray dot;
  i_color temp;
  int ly;

  mm_log((1, "i_circle_aa(im %p, x %d, y %d, rad %.2f, val %p)\n", im, x, y, rad, val));

  i_mmarray_cr(&dot,16*im->ysize);
  make_minmax_list(&dot, x, y, rad);

  for(ly = 0; ly<im->ysize; ly++) {
    int ix, cy, minx = INT_MAX, maxx = INT_MIN;

    /* Find the left/rightmost set subpixels */
    for(cy = 0; cy<16; cy++) {
      frac tmin = dot.data[ly*16+cy].min;
      frac tmax = dot.data[ly*16+cy].max;
      if (tmax == -1) continue;

      if (minx > tmin) minx = tmin;
      if (maxx < tmax) maxx = tmax;
    }

    if (maxx == INT_MIN) continue; /* no work to be done for this row of pixels */

    minx /= 16;
    maxx /= 16;
    for(ix=minx; ix<=maxx; ix++) {
      int cnt = i_pixel_coverage(&dot, ix, ly);
      if (cnt>255) cnt = 255;
      if (cnt) { /* should never be true */
	int ch;
	float ratio = (float)cnt/255.0;
	i_gpix(im, ix, ly, &temp);
	for(ch=0;ch<im->channels; ch++) temp.channel[ch] = (unsigned char)((float)val->channel[ch]*ratio + (float)temp.channel[ch]*(1.0-ratio));
	i_ppix(im, ix, ly, &temp);
      }
    }
  }
  i_mmarray_dst(&dot);
}






void
i_box(i_img *im,int x1,int y1,int x2,int y2,i_color *val) {
  int x,y;
  mm_log((1,"i_box(im* 0x%x,x1 %d,y1 %d,x2 %d,y2 %d,val 0x%x)\n",im,x1,y1,x2,y2,val));
  for(x=x1;x<x2+1;x++) {
    i_ppix(im,x,y1,val);
    i_ppix(im,x,y2,val);
  }
  for(y=y1;y<y2+1;y++) {
    i_ppix(im,x1,y,val);
    i_ppix(im,x2,y,val);
  }
}

void
i_box_filled(i_img *im,int x1,int y1,int x2,int y2,i_color *val) {
  int x,y;
  mm_log((1,"i_box_filled(im* 0x%x,x1 %d,y1 %d,x2 %d,y2 %d,val 0x%x)\n",im,x1,y1,x2,y2,val));
  for(x=x1;x<x2+1;x++) for (y=y1;y<y2+1;y++) i_ppix(im,x,y,val);
}

void
i_box_cfill(i_img *im,int x1,int y1,int x2,int y2,i_fill_t *fill) {
  mm_log((1,"i_box_cfill(im* 0x%x,x1 %d,y1 %d,x2 %d,y2 %d,fill 0x%x)\n",im,x1,y1,x2,y2,fill));

  ++x2;
  if (x1 < 0)
    x1 = 0;
  if (y1 < 0) 
    y1 = 0;
  if (x2 > im->xsize) 
    x2 = im->xsize;
  if (y2 >= im->ysize)
    y2 = im->ysize-1;
  if (x1 >= x2 || y1 > y2)
    return;
  if (im->bits == i_8_bits && fill->fill_with_color) {
    i_color *line = mymalloc(sizeof(i_color) * (x2 - x1)); /* checked 5jul05 tonyc */
    i_color *work = NULL;
    if (fill->combine)
      work = mymalloc(sizeof(i_color) * (x2-x1)); /* checked 5jul05 tonyc */
    while (y1 <= y2) {
      if (fill->combine) {
        i_glin(im, x1, x2, y1, line);
        (fill->fill_with_color)(fill, x1, y1, x2-x1, im->channels, work);
        (fill->combine)(line, work, im->channels, x2-x1);
      }
      else {
        (fill->fill_with_color)(fill, x1, y1, x2-x1, im->channels, line);
      }
      i_plin(im, x1, x2, y1, line);
      ++y1;
    }
    myfree(line);
    if (work)
      myfree(work);
  }
  else {
    i_fcolor *line = mymalloc(sizeof(i_fcolor) * (x2 - x1)); /* checked 5jul05 tonyc */
    i_fcolor *work;
    work = mymalloc(sizeof(i_fcolor) * (x2 - x1)); /* checked 5jul05 tonyc */

    while (y1 <= y2) {
      if (fill->combine) {
        i_glinf(im, x1, x2, y1, line);
        (fill->fill_with_fcolor)(fill, x1, y1, x2-x1, im->channels, work);
        (fill->combinef)(line, work, im->channels, x2-x1);
      }
      else {
        (fill->fill_with_fcolor)(fill, x1, y1, x2-x1, im->channels, line);
      }
      i_plinf(im, x1, x2, y1, line);
      ++y1;
    }
    myfree(line);
    if (work)
      myfree(work);
  }
}


/* 
=item i_line(im, x1, y1, x2, y2, val, endp)

Draw a line to image using bresenhams linedrawing algorithm

   im   - image to draw to
   x1   - starting x coordinate
   y1   - starting x coordinate
   x2   - starting x coordinate
   y2   - starting x coordinate
   val  - color to write to image
   endp - endpoint flag (boolean)

=cut
*/

void
i_line(i_img *im, int x1, int y1, int x2, int y2, i_color *val, int endp) {
  int x, y;
  int dx, dy;
  int p;

  dx = x2 - x1;
  dy = y2 - y1;


  /* choose variable to iterate on */
  if (abs(dx)>abs(dy)) {
    int dx2, dy2, cpy;

    /* sort by x */
    if (x1 > x2) {
      int t;
      t = x1; x1 = x2; x2 = t;
      t = y1; y1 = y2; y2 = t;
    }
    
    dx = abs(dx);
    dx2 = dx*2;
    dy = y2 - y1;

    if (dy<0) {
      dy = -dy;
      cpy = -1;
    } else {
      cpy = 1;
    }
    dy2 = dy*2;
    p = dy2 - dx;

    
    y = y1;
    for(x=x1; x<x2-1; x++) {
      if (p<0) {
        p += dy2;
      } else {
        y += cpy;
        p += dy2-dx2;
      }
      i_ppix(im, x+1, y, val);
    }
  } else {
    int dy2, dx2, cpx;

    /* sort bx y */
    if (y1 > y2) {
      int t;
      t = x1; x1 = x2; x2 = t;
      t = y1; y1 = y2; y2 = t;
    }
    
    dy = abs(dy);
    dx = x2 - x1;
    dy2 = dy*2;

    if (dx<0) {
      dx = -dx;
      cpx = -1;
    } else {
      cpx = 1;
    }
    dx2 = dx*2;
    p = dx2 - dy;

    x = x1;
    
    for(y=y1; y<y2-1; y++) {
      if (p<0) {
        p  += dx2;
      } else {
        x += cpx;
        p += dx2-dy2;
      }
      i_ppix(im, x, y+1, val);
    }
  }
  if (endp) {
    i_ppix(im, x1, y1, val);
    i_ppix(im, x2, y2, val);
  } else {
    if (x1 != x2 || y1 != y2) 
      i_ppix(im, x1, y1, val);
  }
}


void
i_line_dda(i_img *im, int x1, int y1, int x2, int y2, i_color *val) {

  float dy;
  int x;
  
  for(x=x1; x<=x2; x++) {
    dy = y1+ (x-x1)/(float)(x2-x1)*(y2-y1);
    i_ppix(im, x, (int)(dy+0.5), val);
  }
}



























void
i_line_aa3(i_img *im,int x1,int y1,int x2,int y2,i_color *val) {
  i_color tval;
  float alpha;
  float dsec,dfrac;
  int temp,dx,dy,isec,ch;

  mm_log((1,"i_line_aa(im* 0x%x,x1 %d,y1 %d,x2 %d,y2 %d,val 0x%x)\n",im,x1,y1,x2,y2,val));

  dy=y2-y1;
  dx=x2-x1;

  if (abs(dx)>abs(dy)) { /* alpha < 1 */
    if (x2<x1) { temp=x1; x1=x2; x2=temp; temp=y1; y1=y2; y2=temp; }
    alpha=(float)(y2-y1)/(float)(x2-x1);

    dsec=y1;
    while(x1<=x2) {
      isec=(int)dsec;
      dfrac=dsec-isec;
      /*      dfrac=1-(1-dfrac)*(1-dfrac); */
      /* This is something we can play with to try to get better looking lines */

      i_gpix(im,x1,isec,&tval);
      for(ch=0;ch<im->channels;ch++) tval.channel[ch]=(unsigned char)(dfrac*(float)tval.channel[ch]+(1-dfrac)*(float)val->channel[ch]);
      i_ppix(im,x1,isec,&tval);
      
      i_gpix(im,x1,isec+1,&tval);
      for(ch=0;ch<im->channels;ch++) tval.channel[ch]=(unsigned char)((1-dfrac)*(float)tval.channel[ch]+dfrac*(float)val->channel[ch]);
      i_ppix(im,x1,isec+1,&tval);
      
      dsec+=alpha;
      x1++;
    }
  } else {
    if (y2<y1) { temp=y1; y1=y2; y2=temp; temp=x1; x1=x2; x2=temp; }
    alpha=(float)(x2-x1)/(float)(y2-y1);
    dsec=x1;
    while(y1<=y2) {
      isec=(int)dsec;
      dfrac=dsec-isec;
      /*      dfrac=sqrt(dfrac); */
      /* This is something we can play with */
      i_gpix(im,isec,y1,&tval);
      for(ch=0;ch<im->channels;ch++) tval.channel[ch]=(unsigned char)(dfrac*(float)tval.channel[ch]+(1-dfrac)*(float)val->channel[ch]);
      i_ppix(im,isec,y1,&tval);

      i_gpix(im,isec+1,y1,&tval);
      for(ch=0;ch<im->channels;ch++) tval.channel[ch]=(unsigned char)((1-dfrac)*(float)tval.channel[ch]+dfrac*(float)val->channel[ch]);
      i_ppix(im,isec+1,y1,&tval);

      dsec+=alpha;
      y1++;
    }
  }
}




void
i_line_aa(i_img *im, int x1, int y1, int x2, int y2, i_color *val, int endp) {
  int x, y;
  int dx, dy;
  int p;

  dx = x2 - x1;
  dy = y2 - y1;

  /* choose variable to iterate on */
  if (abs(dx)>abs(dy)) {
    int dx2, dy2, cpy;
    
    /* sort by x */
    if (x1 > x2) {
      int t;
      t = x1; x1 = x2; x2 = t;
      t = y1; y1 = y2; y2 = t;
    }
    
    dx = abs(dx);
    dx2 = dx*2;
    dy = y2 - y1;

    if (dy<0) {
      dy = -dy;
      cpy = -1;
    } else {
      cpy = 1;
    }
    dy2 = dy*2;
    p = dy2 - dx2; /* this has to be like this for AA */
    
    y = y1;

    for(x=x1; x<x2-1; x++) {
      int ch;
      i_color tval;
      float t = (dy) ? -(float)(p)/(float)(dx2) : 1;
      float t1, t2;

      if (t<0) t = 0;
      t1 = 1-t;
      t2 = t;

      i_gpix(im,x+1,y,&tval);
      for(ch=0;ch<im->channels;ch++)
	tval.channel[ch]=(unsigned char)(t1*(float)tval.channel[ch]+t2*(float)val->channel[ch]);
      i_ppix(im,x+1,y,&tval);

      i_gpix(im,x+1,y+cpy,&tval);
      for(ch=0;ch<im->channels;ch++)
	tval.channel[ch]=(unsigned char)(t2*(float)tval.channel[ch]+t1*(float)val->channel[ch]);
      i_ppix(im,x+1,y+cpy,&tval);

      if (p<0) {
        p += dy2;
      } else {
        y += cpy;
        p += dy2-dx2;
      }
    }
  } else {
    int dy2, dx2, cpx;

    /* sort bx y */
    if (y1 > y2) {
      int t;
      t = x1; x1 = x2; x2 = t;
      t = y1; y1 = y2; y2 = t;
    }
    
    dy = abs(dy);
    dx = x2 - x1;
    dy2 = dy*2;

    if (dx<0) {
      dx = -dx;
      cpx = -1;
    } else {
      cpx = 1;
    }
    dx2 = dx*2;
    p = dx2 - dy2; /* this has to be like this for AA */

    x = x1;
    
    for(y=y1; y<y2-1; y++) {
      int ch;
      i_color tval;
      float t = (dx) ? -(float)(p)/(float)(dy2) : 1;
      float t1, t2;
      
      if (t<0) t = 0;
      t1 = 1-t;
      t2 = t;

      i_gpix(im,x,y+1,&tval);
      for(ch=0;ch<im->channels;ch++)
	tval.channel[ch]=(unsigned char)(t1*(float)tval.channel[ch]+t2*(float)val->channel[ch]);
      i_ppix(im,x,y+1,&tval);

      i_gpix(im,x+cpx,y+1,&tval);
      for(ch=0;ch<im->channels;ch++)
	tval.channel[ch]=(unsigned char)(t2*(float)tval.channel[ch]+t1*(float)val->channel[ch]);
      i_ppix(im,x+cpx,y+1,&tval);

      if (p<0) {
        p  += dx2;
      } else {
        x += cpx;
        p += dx2-dy2;
      }
    }
  }


  if (endp) {
    i_ppix(im, x1, y1, val);
    i_ppix(im, x2, y2, val);
  } else {
    if (x1 != x2 || y1 != y2) 
      i_ppix(im, x1, y1, val);
  }
}



static double
perm(int n,int k) {
  double r;
  int i;
  r=1;
  for(i=k+1;i<=n;i++) r*=i;
  for(i=1;i<=(n-k);i++) r/=i;
  return r;
}


/* Note in calculating t^k*(1-t)^(n-k) 
   we can start by using t^0=1 so this simplifies to
   t^0*(1-t)^n - we want to multiply that with t/(1-t) each iteration
   to get a new level - this may lead to errors who knows lets test it */

void
i_bezier_multi(i_img *im,int l,double *x,double *y,i_color *val) {
  double *bzcoef;
  double t,cx,cy;
  int k,i;
  int lx = 0,ly = 0;
  int n=l-1;
  double itr,ccoef;

  /* this is the same size as the x and y arrays, so shouldn't overflow */
  bzcoef=mymalloc(sizeof(double)*l); /* checked 5jul05 tonyc */
  for(k=0;k<l;k++) bzcoef[k]=perm(n,k);
  ICL_info(val);


  /*  for(k=0;k<l;k++) printf("bzcoef: %d -> %f\n",k,bzcoef[k]); */
  i=0;
  for(t=0;t<=1;t+=0.005) {
    cx=cy=0;
    itr=t/(1-t);
    ccoef=pow(1-t,n);
    for(k=0;k<l;k++) {
      /*      cx+=bzcoef[k]*x[k]*pow(t,k)*pow(1-t,n-k); 
	      cy+=bzcoef[k]*y[k]*pow(t,k)*pow(1-t,n-k);*/

      cx+=bzcoef[k]*x[k]*ccoef;
      cy+=bzcoef[k]*y[k]*ccoef;
      ccoef*=itr;
    }
    /*    printf("%f -> (%d,%d)\n",t,(int)(0.5+cx),(int)(0.5+cy)); */
    if (i++) { 
      i_line_aa(im,lx,ly,(int)(0.5+cx),(int)(0.5+cy),val, 1);
    }
      /*     i_ppix(im,(int)(0.5+cx),(int)(0.5+cy),val); */
    lx=(int)(0.5+cx);
    ly=(int)(0.5+cy);
  }
  ICL_info(val);
  myfree(bzcoef);
}

/* Flood fill 

   REF: Graphics Gems I. page 282+

*/

/* This should be moved into a seperate file? */

/* This is the truncation used:
   
   a double is multiplied by 16 and then truncated.
   This means that 0 -> 0
   So a triangle of (0,0) (10,10) (10,0) Will look like it's
   not filling the (10,10) point nor the (10,0)-(10,10)  line segment

*/


/* Flood fill algorithm - based on the Ken Fishkins (pixar) gem in 
   graphics gems I */

/*
struct stc {
  int mylx,myrx; 
  int dadlx,dadrx;
  int myy;
  int mydirection;
};

Not used code???
*/


struct stack_element {
  int myLx,myRx;
  int dadLx,dadRx;
  int myY;
  int myDirection;
};


/* create the link data to put push onto the stack */

static
struct stack_element*
crdata(int left,int right,int dadl,int dadr,int y, int dir) {
  struct stack_element *ste;
  ste              = mymalloc(sizeof(struct stack_element)); /* checked 5jul05 tonyc */
  ste->myLx        = left;
  ste->myRx        = right;
  ste->dadLx       = dadl;
  ste->dadRx       = dadr;
  ste->myY         = y;
  ste->myDirection = dir;
  return ste;
}

/* i_ccomp compares two colors and gives true if they are the same */

static int
i_ccomp(i_color *val1,i_color *val2,int ch) {
  int i;
  for(i=0;i<ch;i++) if (val1->channel[i] !=val2->channel[i]) return 0;
  return 1;
}


static int
i_lspan(i_img *im, int seedx, int seedy, i_color *val) {
  i_color cval;
  while(1) {
    if (seedx-1 < 0) break;
    i_gpix(im,seedx-1,seedy,&cval);
    if (!i_ccomp(val,&cval,im->channels)) break;
    seedx--;
  }
  return seedx;
}

static int
i_rspan(i_img *im, int seedx, int seedy, i_color *val) {
  i_color cval;
  while(1) {
    if (seedx+1 > im->xsize-1) break;
    i_gpix(im,seedx+1,seedy,&cval);
    if (!i_ccomp(val,&cval,im->channels)) break;
    seedx++;
  }
  return seedx;
}

/* Macro to create a link and push on to the list */

#define ST_PUSH(left,right,dadl,dadr,y,dir) do {                 \
  struct stack_element *s = crdata(left,right,dadl,dadr,y,dir);  \
  llist_push(st,&s);                                             \
} while (0)

/* pops the shadow on TOS into local variables lx,rx,y,direction,dadLx and dadRx */
/* No overflow check! */
 
#define ST_POP() do {         \
  struct stack_element *s;    \
  llist_pop(st,&s);           \
  lx        = s->myLx;        \
  rx        = s->myRx;        \
  dadLx     = s->dadLx;       \
  dadRx     = s->dadRx;       \
  y         = s->myY;         \
  direction = s->myDirection; \
  myfree(s);                  \
} while (0)

#define ST_STACK(dir,dadLx,dadRx,lx,rx,y) do {                    \
  int pushrx = rx+1;                                              \
  int pushlx = lx-1;                                              \
  ST_PUSH(lx,rx,pushlx,pushrx,y+dir,dir);                         \
  if (rx > dadRx)                                                 \
    ST_PUSH(dadRx+1,rx,pushlx,pushrx,y-dir,-dir);                 \
  if (lx < dadLx) ST_PUSH(lx,dadLx-1,pushlx,pushrx,y-dir,-dir);   \
} while (0)

#define SET(x,y) btm_set(btm,x,y)

/* INSIDE returns true if pixel is correct color and we haven't set it before. */
#define INSIDE(x,y) ((!btm_test(btm,x,y) && ( i_gpix(im,x,y,&cval),i_ccomp(&val,&cval,channels)  ) ))



/* The function that does all the real work */

static struct i_bitmap *
i_flood_fill_low(i_img *im,int seedx,int seedy,
                 int *bxminp, int *bxmaxp, int *byminp, int *bymaxp) {

  /*
    int lx,rx;
    int y;
    int direction;
    int dadLx,dadRx;
    int wasIn=0;
  */
  int ltx, rtx;
  int tx = 0;

  int bxmin = seedx;
  int bxmax = seedx;
  int bymin = seedy;
  int bymax = seedy;

  struct llist *st;
  struct i_bitmap *btm;

  int channels,xsize,ysize;
  i_color cval,val;

  channels = im->channels;
  xsize    = im->xsize;
  ysize    = im->ysize;

  btm = btm_new(xsize, ysize);
  st  = llist_new(100, sizeof(struct stack_element*));

  /* Get the reference color */
  i_gpix(im, seedx, seedy, &val);

  /* Find the starting span and fill it */
  ltx = i_lspan(im, seedx, seedy, &val);
  rtx = i_rspan(im, seedx, seedy, &val);
  for(tx=ltx; tx<=rtx; tx++) SET(tx, seedy);

  ST_PUSH(ltx, rtx, ltx, rtx, seedy+1,  1);
  ST_PUSH(ltx, rtx, ltx, rtx, seedy-1, -1);

  while(st->count) {
    /* Stack variables */
    int lx,rx;
    int dadLx,dadRx;
    int y;
    int direction;

    int x;
    int wasIn=0;

    ST_POP(); /* sets lx, rx, dadLx, dadRx, y, direction */


    if (y<0 || y>ysize-1) continue;
    if (bymin > y) bymin=y; /* in the worst case an extra line */
    if (bymax < y) bymax=y; 


    x = lx+1;
    if ( lx >= 0 && (wasIn = INSIDE(lx, y)) ) {
      SET(lx, y);
      lx--;
      while(INSIDE(lx, y) && lx > 0) {
	SET(lx,y);
	lx--;
      }
    }

    if (bxmin > lx) bxmin = lx;
    while(x <= xsize-1) {
      /*  printf("x=%d\n",x); */
      if (wasIn) {
	
	if (INSIDE(x, y)) {
	  /* case 1: was inside, am still inside */
	  SET(x,y);
	} else {
	  /* case 2: was inside, am no longer inside: just found the
	     right edge of a span */
	  ST_STACK(direction, dadLx, dadRx, lx, (x-1), y);

	  if (bxmax < x) bxmax = x;
	  wasIn=0;
	}
      } else {
	if (x > rx) goto EXT;
	if (INSIDE(x, y)) {
	  SET(x, y);
	  /* case 3: Wasn't inside, am now: just found the start of a new run */
	  wasIn = 1;
	    lx = x;
	} else {
	  /* case 4: Wasn't inside, still isn't */
	}
      }
      x++;
    }
  EXT: /* out of loop */
    if (wasIn) {
      /* hit an edge of the frame buffer while inside a run */
      ST_STACK(direction, dadLx, dadRx, lx, (x-1), y);
      if (bxmax < x) bxmax = x;
    }
  }

  llist_destroy(st);

  *bxminp = bxmin;
  *bxmaxp = bxmax;
  *byminp = bymin;
  *bymaxp = bymax;

  return btm;
}




undef_int
i_flood_fill(i_img *im, int seedx, int seedy, i_color *dcol) {
  int bxmin, bxmax, bymin, bymax;
  struct i_bitmap *btm;
  int x, y;

  i_clear_error();
  if (seedx < 0 || seedx >= im->xsize ||
      seedy < 0 || seedy >= im->ysize) {
    i_push_error(0, "i_flood_cfill: Seed pixel outside of image");
    return 0;
  }

  btm = i_flood_fill_low(im, seedx, seedy, &bxmin, &bxmax, &bymin, &bymax);

  for(y=bymin;y<=bymax;y++)
    for(x=bxmin;x<=bxmax;x++)
      if (btm_test(btm,x,y)) 
	i_ppix(im,x,y,dcol);
  btm_destroy(btm);
  return 1;
}



undef_int
i_flood_cfill(i_img *im, int seedx, int seedy, i_fill_t *fill) {
  int bxmin, bxmax, bymin, bymax;
  struct i_bitmap *btm;
  int x, y;
  int start;

  i_clear_error();
  
  if (seedx < 0 || seedx >= im->xsize ||
      seedy < 0 || seedy >= im->ysize) {
    i_push_error(0, "i_flood_cfill: Seed pixel outside of image");
    return 0;
  }

  btm = i_flood_fill_low(im, seedx, seedy, &bxmin, &bxmax, &bymin, &bymax);

  if (im->bits == i_8_bits && fill->fill_with_color) {
    /* bxmax/bxmin are inside the image, hence this won't overflow */
    i_color *line = mymalloc(sizeof(i_color) * (bxmax - bxmin)); /* checked 5jul05 tonyc */
    i_color *work = NULL;
    if (fill->combine)
      work = mymalloc(sizeof(i_color) * (bxmax - bxmin)); /* checked 5jul05 tonyc */

    for(y=bymin; y<=bymax; y++) {
      x = bxmin;
      while (x < bxmax) {
        while (x < bxmax && !btm_test(btm, x, y)) {
          ++x;
        }
        if (btm_test(btm, x, y)) {
          start = x;
          while (x < bxmax && btm_test(btm, x, y)) {
            ++x;
          }
          if (fill->combine) {
            i_glin(im, start, x, y, line);
            (fill->fill_with_color)(fill, start, y, x-start, im->channels, 
                                    work);
            (fill->combine)(line, work, im->channels, x-start);
          }
          else {
            (fill->fill_with_color)(fill, start, y, x-start, im->channels, 
                                    line);
          }
          i_plin(im, start, x, y, line);
        }
      }
    }
    myfree(line);
    if (work)
      myfree(work);
  }
  else {
    /* bxmax/bxmin are inside the image, hence this won't overflow */
    i_fcolor *line = mymalloc(sizeof(i_fcolor) * (bxmax - bxmin)); /* checked 5jul05 tonyc */
    i_fcolor *work = NULL;
    if (fill->combinef)
      work = mymalloc(sizeof(i_fcolor) * (bxmax - bxmin)); /* checked 5jul05 tonyc */
    
    for(y=bymin;y<=bymax;y++) {
      x = bxmin;
      while (x < bxmax) {
        while (x < bxmax && !btm_test(btm, x, y)) {
          ++x;
        }
        if (btm_test(btm, x, y)) {
          start = x;
          while (x < bxmax && btm_test(btm, x, y)) {
            ++x;
          }
          if (fill->combinef) {
            i_glinf(im, start, x, y, line);
            (fill->fill_with_fcolor)(fill, start, y, x-start, im->channels, 
                                    work);
            (fill->combinef)(line, work, im->channels, x-start);
          }
          else {
            (fill->fill_with_fcolor)(fill, start, y, x-start, im->channels, 
                                    line);
          }
          i_plinf(im, start, x, y, line);
        }
      }
    }
    myfree(line);
    if (work)
      myfree(work);
  }

  btm_destroy(btm);
  return 1;
}
