#include <_types/_uint16_t.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum pixel_t { PIXEL_U16 };
enum image_data_t {
  // Uninitialised data
  NONE_DATA_T,
  // PGM RAW Bayer Data
  PIXEL_RAW_T,
  // Standard RGB with 16-bit per channel.
  PIXEL_RGB_T,
};

struct image {
  unsigned int height;
  unsigned int width;
  void *data;
  enum image_data_t data_t; // TODO: Add plumbing for this.
};

struct pixel_raw_t {
  uint16_t v;
};

struct pixel_rgb_t {
  uint16_t r;
  uint16_t g;
  uint16_t b;
};

void print_pixel(struct pixel_rgb_t *in) {
  if (in == NULL) {
    return;
  }
  printf("Pixel %p: (%u, %u, %u)\n", &in, in->r, in->g, in->b);
}

bool pixel_eq(struct pixel_rgb_t *a, struct pixel_rgb_t *b) {
  return ((a->r == b->r) && (a->g == b->g) && (a->b == b->b));
}

bool init_img_buffer(struct image *img, unsigned int height, unsigned int width,
                     enum image_data_t data_t) {
  switch (data_t) {
  case PIXEL_RAW_T:
    img->data = malloc(height * width * sizeof(struct pixel_raw_t));
    break;
  case PIXEL_RGB_T:
    img->data = malloc(height * width * sizeof(struct pixel_rgb_t));
    break;
  default:
    printf("Invalid IMAGE_DATA_T enum provided for img init");
    return false;
  }

  if (img->data) {
    img->data_t = data_t;
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
  img->data_t = NONE_DATA_T;
}

void print_img_buffer(struct image *img) {
  printf("Image height: %u\n", img->height);
  printf("Image width: %u\n", img->width);
  printf("Data Pointer: %p\n", img->data);
  if (img->data) {
    printf("Buffer size in pixels: %zd\n",
           (img->height * img->width) / sizeof(struct pixel_rgb_t));
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

bool write_img_pixel_u16(struct image *img_out, struct pixel_rgb_t *in,
                         unsigned int x, unsigned int y) {
  if (y >= img_out->height || x >= img_out->width ||
      img_out->data_t != PIXEL_RGB_T) {
    return false;
  }

  unsigned int index = calculate_index(img_out, x, y);
  struct pixel_rgb_t *pixels = (struct pixel_rgb_t *)img_out->data;
  memcpy((struct pixel_rgb_t *)pixels + index, in, sizeof(struct pixel_rgb_t));
  return true;
}

bool read_img_pixel_u16(struct image *img_in, struct pixel_rgb_t *out,
                        unsigned int x, unsigned int y) {
  if (y >= img_in->height || x >= img_in->width ||
      img_in->data_t != PIXEL_RGB_T) {
    return false;
  }

  unsigned int index = calculate_index(img_in, x, y);
  struct pixel_rgb_t *pixels = (struct pixel_rgb_t *)img_in->data;
  memcpy(out, (struct pixel_rgb_t *)pixels + index, sizeof(struct pixel_rgb_t));
  return true;
}

void test_pixel_u16() {
  struct image img = {0};
  struct pixel_rgb_t my_pixel = (struct pixel_rgb_t){
      .r = 123,
      .g = 100,
      .b = 50,
  };
  struct pixel_rgb_t read_pixel = {0};

  print_pixel(&my_pixel);
  print_pixel(&read_pixel);

  if (!init_img_buffer(&img, 100, 100, PIXEL_RGB_T))
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

/* ChatGPT wrote this function. */
void parse_ws(FILE *fp) {
  int curr_char;
  while ((curr_char = fgetc(fp)) != EOF) {
    if (isspace(curr_char)) {
      continue;
    }

    if (curr_char == '#') {
      while ((curr_char = fgetc(fp)) != '\n')
        ;
      continue;
    }

    ungetc(curr_char, fp);
    break;
  }
}

/* I wrote this function. */
unsigned short parse_pgm_int(FILE *fp) {
  char buffer[10] = {0};
  int curr_char;

  while ((curr_char = fgetc(fp)) != EOF) {

    if (isspace(curr_char)) {
      break;
    }

    sprintf(buffer, "%s%c", buffer, (char)curr_char);
  }

  ungetc(curr_char, fp);
  return (unsigned short)atoi(buffer);
}

/**
 * Read a PGM containing the raw Bayer data into an image buffer.
 *
 * Important information for RPi Camera from dcraw:
 * Scaling with darkness 4096, saturation 65535, and
 * multipliers 1.018250 1.000000 2.160646 1.000000
 */
bool read_pgm(const char *path, struct image *img) {
  char buffer[100] = {0};
  int curr_char, parsed_value;
  unsigned int height, width, maxvalue;
  FILE *fp;

  if (!img) {
    return false;
  }

  // Clear out the image data
  if (img->data) {
    printf("Warning: overwriting existing data in image buffer.");
  }

  free_img_buffer(img);

  fp = fopen(path, "rb");
  if (!fp) {
    printf("Couldn't open PGM file.");
    return false;
  }

  fread(buffer, sizeof(char), 2, fp);
  if (!strstr(buffer, "P5")) {
    printf("PGM file is missing the 'P5' magic number.");
    printf("%s\n", buffer);
    return false;
  }

  memset(buffer, 0, sizeof(buffer));

  parse_ws(fp);
  width = parse_pgm_int(fp);
  parse_ws(fp);
  height = parse_pgm_int(fp);
  parse_ws(fp);
  maxvalue = parse_pgm_int(fp);
  fgetc(fp);

  if (!init_img_buffer(img, height, width, PIXEL_RAW_T)) {
    printf("Error creating image buffer for RAW PGM file.");
    return false;
  }

  printf("h: %u, w: %u, v: %u\n", height, width, maxvalue);

  for (unsigned int px = 0; px < height * width; px++) {
    uint16_t value;
    uint8_t v[2];
    unsigned int index;
    int parsed_char;
    struct pixel_raw_t pixel = {0};

    fread(&value, sizeof(uint16_t), 1, fp);
    pixel.v = value;
    memcpy((struct pixel_raw_t *)img->data + px, &pixel,
           sizeof(struct pixel_raw_t));
  }

  fclose(fp);
  return true;
}

/**
 * TODO: We will have to make sure that the data format within the image buffer
 * is correct.
 */
bool write_ppm(const char *path, struct image *img) {
  char int_buffer[60] = {0};
  FILE *fp = fopen(path, "wb");
  if (!fp || !img || !img->data) {
    return false;
  }

  /* The old PPM format has a 'maxval' field of 255, but newer formats allow
   * 16-bit values now. */
  fputs("P6\n", fp);
  sprintf(int_buffer, "%u %u\n", img->width, img->height);
  fputs(int_buffer, fp);
  fputs("65535\n", fp);

  if (img->data_t == PIXEL_RAW_T) {
    for (int px = 0; px < img->height * img->width; px++) {
      struct pixel_raw_t pxl = (struct pixel_raw_t){0};
      struct pixel_raw_t *data = img->data;
      memcpy(&pxl, (struct pixel_raw_t *)img->data + px,
             sizeof(struct pixel_raw_t));
      fwrite(&pxl.v, sizeof(uint16_t), 1, fp);
      fwrite(&pxl.v, sizeof(uint16_t), 1, fp);
      fwrite(&pxl.v, sizeof(uint16_t), 1, fp);
    }
  } else if (img->data_t == PIXEL_RGB_T) {
    for (int h = 0; h < img->height; h++) {
      for (int w = 0; w < img->width; w++) {
        char pixel_buf[100] = {0};
        struct pixel_rgb_t pxl = (struct pixel_rgb_t){0};
        read_img_pixel_u16(img, &pxl, w, h);
        fwrite(&pxl.r, sizeof(uint16_t), 1, fp);
        fwrite(&pxl.g, sizeof(uint16_t), 1, fp);
        fwrite(&pxl.b, sizeof(uint16_t), 1, fp);
      }
    }
  }
  fclose(fp);
  return true;
}

bool export_image(const char *file, struct image *img) {
  if (img) {
    switch (img->data_t) {
    case PIXEL_RGB_T:
      return write_ppm(file, img);
    default:
      printf("Unsupported export option\n");
      return false;
    }
  }

  return false;
}

void test_ppm_write() {
  struct image my_buf = {0};
  unsigned int limit = 6552;
  init_img_buffer(&my_buf, limit / 2, limit, PIXEL_RGB_T);

  for (int h = 0; h < my_buf.height; h++) {
    for (int w = 0; w < my_buf.width; w++) {
      struct pixel_rgb_t pxl = (struct pixel_rgb_t){
          .r = (uint16_t)h + w,
          .g = (uint16_t)h + w,
          .b = (uint16_t)h + w,
      };
      write_img_pixel_u16(&my_buf, &pxl, w, h);
    }
  }

  export_image("../res/out.ppm", &my_buf);

  free_img_buffer(&my_buf);
}

int main() {
  // test_ppm_write();
  struct image img = (struct image){0};
  read_pgm("../res/rpi_test.pgm", &img);
  write_ppm("../res/rpi_test.ppm", &img);
  free_img_buffer(&img);
  return 0;
}
