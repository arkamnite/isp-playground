#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct img_buffer {
  unsigned int height;
  unsigned int width;
  void *data;
};

struct pixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

void print_pixel(struct pixel *in) {
  if (in == NULL) {
    return;
  }
  printf("Pixel %p: (%d, %d, %d)\n", &in, in->r, in->g, in->b);
}

bool pixel_eq(struct pixel *a, struct pixel *b) {
  return ((a->r == b->r) && (a->g == b->g) && (a->b == b->b));
}

bool init_img_buffer(struct img_buffer *img, unsigned int height,
                     unsigned int width) {
  img->data = malloc(height * width * sizeof(struct pixel));

  if (img->data) {
    img->height = height;
    img->width = width;
  }

  return (img->data);
}

void free_img_buffer(struct img_buffer *img) {
  // printf("data pointer: %p\n", img->data);
  img->height = 0;
  img->width = 0;
  free(img->data);
  img->data = NULL;
}

void print_img_buffer(struct img_buffer *img) {
  printf("Image height: %u\n", img->height);
  printf("Image width: %u\n", img->width);
  printf("Data Pointer: %p\n", img->data);
  if (img->data) {
    printf("Buffer size in pixels: %zd\n",
           (img->height * img->width) / sizeof(struct pixel));
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
unsigned int calculate_index(struct img_buffer *img, unsigned int x,
                             unsigned int y) {
  return y * img->width + x;
}

bool write_to_buf(struct img_buffer *img_out, struct pixel *in, unsigned int x,
                  unsigned int y) {
  if (y >= img_out->height || x >= img_out->width) {
    return false;
  }

  unsigned int index = calculate_index(img_out, x, y);
  struct pixel *pixels = (struct pixel *)img_out->data;
  memcpy(pixels + index, in, sizeof(struct pixel));
  return true;
}

bool read_buf(struct img_buffer *img_in, struct pixel *out, unsigned int x,
              unsigned int y) {
  if (y >= img_in->height || x >= img_in->width) {
    return false;
  }

  unsigned int index = calculate_index(img_in, x, y);
  struct pixel *pixels = (struct pixel *)img_in->data;
  memcpy(out, pixels + index, sizeof(struct pixel));
  return true;
}

void test_pixel() {
  struct img_buffer img = {0};
  struct pixel my_pixel = (struct pixel){
      .r = 123,
      .g = 100,
      .b = 50,
  };
  struct pixel read_pixel = {0};

  print_pixel(&my_pixel);
  print_pixel(&read_pixel);

  if (!init_img_buffer(&img, 100, 100))
    goto cleanup;

  if (!write_to_buf(&img, &my_pixel, 0, 0))
    goto cleanup;

  if (!read_buf(&img, &read_pixel, 0, 0))
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

bool write_ppm(const char *path, struct img_buffer *img) {
  char int_buffer[60] = {0};
  FILE *fp = fopen(path, "w");
  if (!fp || !img || !img->data) {
    return false;
  }

  /* PPM format has a 'maxval' field of 255. */
  fputs("P6\n", fp);
  sprintf(int_buffer, "%u %u\n", img->width, img->height);
  fputs(int_buffer, fp);
  fputs("255\n", fp);

  for (int h = 0; h < img->height; h++) {
    for (int w = 0; w < img->width; w++) {
      char pixel_buf[100] = {0};
      struct pixel pxl = (struct pixel){0};
      read_buf(img, &pxl, w, h);
      sprintf(pixel_buf, "%u %u %u", pxl.r, pxl.g, pxl.b);
      fputs(pixel_buf, fp);
    }
    fputs("\n", fp);
  }
  fclose(fp);
  return true;
}

int main() {
  // create buffer on the stack
  struct img_buffer my_buf = {0};
  unsigned int limit = 100;
  init_img_buffer(&my_buf, limit, limit);

  for (int h = 0; h < my_buf.height; h++) {
    for (int w = 0; w < my_buf.width; w++) {
      struct pixel pxl = (struct pixel){
          .r = (char)h + w,
          .g = (char)h + w,
          .b = (char)h + w,
      };
      write_to_buf(&my_buf, &pxl, w, h);
    }
  }

  write_ppm("../out.ppm", &my_buf);
  // test_pixel();
  free_img_buffer(&my_buf);
  return 0;
}
