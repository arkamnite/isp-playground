#ifndef IMAGE_H
#define IMAGE_H

/* ================================================== */
/* basic types to represent pipeline state and images */
/* ================================================== */

/* Standard type for pixel representation. */
struct pixel_i32 {
  int _r;
  int _g;
  int _b;
};

/* Common state for all pipeline stages. */
struct pipeline_state {
  // image data
  unsigned int _w;
  unsigned int _h;

  // all of our pixels
};

#endif
