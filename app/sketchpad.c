#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum pixel_t { PIXEL_U16 };

struct image {
  unsigned int height;
  unsigned int width;
  void *data;
  enum pixel_t data_t;
};

struct pixel_u16 {
  uint16_t r;
  uint16_t g;
  uint16_t b;
};

void print_pixel(struct pixel_u16 *in) {
  if (in == NULL) {
    return;
  }
  printf("Pixel %p: (%u, %u, %u)\n", &in, in->r, in->g, in->b);
}

bool pixel_eq(struct pixel_u16 *a, struct pixel_u16 *b) {
  return ((a->r == b->r) && (a->g == b->g) && (a->b == b->b));
}

bool init_img_buffer(struct image *img, unsigned int height,
                     unsigned int width) {
  img->data = malloc(height * width * sizeof(struct pixel_u16));

  if (img->data) {
    img->height = height;
    img->width = width;
  }

  return (img->data);
}

void free_img_buffer(struct image *img) {
  // printf("data pointer: %p\n", img->data);
  img->height = 0;
  img->width = 0;
  free(img->data);
  img->data = NULL;
}

void print_img_buffer(struct image *img) {
  printf("Image height: %u\n", img->height);
  printf("Image width: %u\n", img->width);
  printf("Data Pointer: %p\n", img->data);
  if (img->data) {
    printf("Buffer size in pixels: %zd\n",
           (img->height * img->width) / sizeof(struct pixel_u16));
  }
}

/**
 * Maps the following example matrix
 *
 *   x ->
 * y
 * || 0 1 2
 * \/ 3 4 5
 *    6 7 8
 *
 * Into this:
 * 0 1 2 3 4 5 6 7 8
 */
unsigned int calculate_index(struct image *img, unsigned int x,
                             unsigned int y) {
  return y * img->width + x;
}

bool write_img_pixel_u16(struct image *img_out, struct pixel_u16 *in,
                         unsigned int x, unsigned int y) {
  if (y >= img_out->height || x >= img_out->width) {
    return false;
  }

  unsigned int index = calculate_index(img_out, x, y);
  struct pixel_u16 *pixels = (struct pixel_u16 *)img_out->data;
  memcpy(pixels + index, in, sizeof(struct pixel_u16));
  return true;
}

bool read_img_pixel_u16(struct image *img_in, struct pixel_u16 *out,
                        unsigned int x, unsigned int y) {
  if (y >= img_in->height || x >= img_in->width) {
    return false;
  }

  unsigned int index = calculate_index(img_in, x, y);
  struct pixel_u16 *pixels = (struct pixel_u16 *)img_in->data;
  memcpy(out, pixels + index, sizeof(struct pixel_u16));
  return true;
}

void test_pixel_u16() {
  struct image img = {0};
  struct pixel_u16 my_pixel = (struct pixel_u16){
      .r = 123,
      .g = 100,
      .b = 50,
  };
  struct pixel_u16 read_pixel = {0};

  print_pixel(&my_pixel);
  print_pixel(&read_pixel);

  if (!init_img_buffer(&img, 100, 100))
    goto cleanup;

  if (!write_img_pixel_u16(&img, &my_pixel, 0, 0))
    goto cleanup;

  if (!read_img_pixel_u16(&img, &read_pixel, 0, 0))
    goto cleanup;

  print_pixel(&my_pixel);
  print_pixel(&read_pixel);

  if (!pixel_eq(&my_pixel, &read_pixel))
    goto cleanup;

  return;

cleanup:
  printf("test_pixel() failed!");
  free_img_buffer(&img);
}

/**
 * TODO: We will have to make sure that the data format within the image buffer
 * is correct.
 */
bool write_ppm(const char *path, struct image *img) {
  char int_buffer[60] = {0};
  FILE *fp = fopen(path, "w");
  if (!fp || !img || !img->data) {
    return false;
  }

  /* The old PPM format has a 'maxval' field of 255, but newer formats allow
   * 16-bit values now. */
  fputs("P6\n", fp);
  sprintf(int_buffer, "%u %u\n", img->width, img->height);
  fputs(int_buffer, fp);
  fputs("65535\n", fp);

  for (int h = 0; h < img->height; h++) {
    for (int w = 0; w < img->width; w++) {
      char pixel_buf[100] = {0};
      struct pixel_u16 pxl = (struct pixel_u16){0};
      read_img_pixel_u16(img, &pxl, w, h);
      sprintf(pixel_buf, "%u %u %u", pxl.r, pxl.g, pxl.b);
      fputs(pixel_buf, fp);
    }
    fputs("\n", fp);
  }
  fclose(fp);
  return true;
}

// TODO: extract Bayer RAW from DNG image.
// Need to iterate over headers, find the right SubIFDs, and then follow the
// breadcrumbs to the RAW data.
bool dng_to_bayer(const char *file, struct image *img) { return false; }

int main() {
  struct image my_buf = {0};
  unsigned int limit = 6552;
  init_img_buffer(&my_buf, limit / 2, limit);

  for (int h = 0; h < my_buf.height; h++) {
    for (int w = 0; w < my_buf.width; w++) {
      struct pixel_u16 pxl = (struct pixel_u16){
          .r = (uint16_t)h + w,
          .g = (uint16_t)h + w,
          .b = (uint16_t)h + w,
      };
      write_img_pixel_u16(&my_buf, &pxl, w, h);
    }
  }

  write_ppm("../res/out.ppm", &my_buf);

  // test_pixel();
  free_img_buffer(&my_buf);
  return 0;
}
