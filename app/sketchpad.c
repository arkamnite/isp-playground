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
  char r;
  char g;
  char b;
};

void print_pixel(struct pixel *in) {
  if (in == NULL) {
    return;
  }
  printf("Pixel %p: (%d, %d, %d)\n", &in, in->r, in->g, in->b);
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

int main() {
  // create buffer on the stack
  struct img_buffer my_buf = {0};
  // print_img_buffer(&my_buf);
  // printf("init result: %d\n", init_img_buffer(&my_buf, 100, 100));
  // print_img_buffer(&my_buf);
  // free_img_buffer(&my_buf);
  // print_img_buffer(&my_buf);

  struct pixel my_pixel = (struct pixel){
      .r = 123,
      .g = 100,
      .b = 50,
  };
  struct pixel read_pixel = {0};
  print_pixel(&my_pixel);
  print_pixel(&read_pixel);

  printf("init result: %d\n", init_img_buffer(&my_buf, 100, 100));
  write_to_buf(&my_buf, &my_pixel, 0, 0);
  read_buf(&my_buf, &read_pixel, 0, 0);
  print_pixel(&my_pixel);
  print_pixel(&read_pixel);
  free_img_buffer(&my_buf);

  return 0;
}
