#include <_types/_uint16_t.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Set these environment variables to any value in order to
 * enable debug logs for that pass.
 */
#define PASS_NORM_DEBUG_STR "DEBUG_PASS_NORM"
#define PASS_WB_DEBUG_STR "DEBUG_PASS_WB"
#define PASS_DEBAYER_DEBUG_STR "DEBUG_PASS_DEBAYER"

enum pixel_t { PIXEL_U16 };
enum image_data_t {
  // Uninitialised data
  NONE_DATA_T,
  // PGM RAW Bayer Data
  PIXEL_RAW_T,
  // Normalised pixel data, using a single 32-bit float.
  PIXEL_F32_T,
  // Pixel data using 3 channels of 32-bit float data each.
  PIXEL_F32_V3_T,
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

struct pixel_f32_t {
  float v;
};

struct pixel_f32_v3_t {
  float r;
  float g;
  float b;
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
  size_t size = height * width;
  switch (data_t) {
  case PIXEL_RAW_T:
    img->data = malloc(size * sizeof(struct pixel_raw_t));
    break;
  case PIXEL_F32_T:
    img->data = malloc(size * sizeof(struct pixel_f32_t));
    break;
  case PIXEL_RGB_T:
    img->data = malloc(size * sizeof(struct pixel_rgb_t));
    break;
  case PIXEL_F32_V3_T:
    img->data = malloc(size * sizeof(struct pixel_f32_v3_t));
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
  } else if (img->data_t == PIXEL_F32_V3_T) {
    // We will have to de-normalise the data here back to the PPM range.
    for (int px = 0; px < img->height * img->width; px++) {
      struct pixel_f32_v3_t *data = img->data;
      struct pixel_f32_v3_t pxl = (struct pixel_f32_v3_t){0};
      memcpy(&pxl, (struct pixel_f32_v3_t *)img->data + px,
             sizeof(struct pixel_f32_v3_t));
      unsigned int r = pxl.r * (65535 - 4096) + 4096;
      unsigned int g = pxl.g * (65535 - 4096) + 4096;
      unsigned int b = pxl.b * (65535 - 4096) + 4096;
      fwrite(&r, sizeof(uint16_t), 1, fp);
      fwrite(&g, sizeof(uint16_t), 1, fp);
      fwrite(&b, sizeof(uint16_t), 1, fp);
    }

  } else {
    printf("Unsupported data type for image buffer!");
    fclose(fp);
    return false;
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

/* ================================================= */
/*
 * Passes
 * 1. Linearlisation & Normalisation
 * 2. White Balancing
 * 3. Demosaicing / De-Bayering
 * 4. Colour Space Correction
 * 5. Brightness and Contrast Control
 */
/* ================================================= */

/*
 * Linearisation & Normalisation
 *
 * Whilst we assume that dcraw has linearised our image
 * for us, the RAW image may require normalising if there
 * is any offset and scaling to the values. We need to apply
 * an affine transformation based on some of the values given
 * to us by dcraw when generating the RAW Bayer image.
 *
 * In this example, we need to normalise from [Black,White] to [0,1]
 * in order to normalise these pixels. This is an involved routine
 * which requires us to change from using integers to using floats,
 * otherwise we're going to have some pretty useless data...
 */
bool apply_normalisation(struct image *img, uint16_t b, uint16_t w) {
  if (img->data_t != PIXEL_RAW_T) {
    return false;
  }

  if (w <= b) {
    printf("Invalid B/W arguments provided for normalisation!");
    return false;
  }

  // Find out if we are printing debugs
  bool print_debugs = getenv(PASS_NORM_DEBUG_STR) != NULL;

  // Copy the image data buffer to here first
  size_t raw_data_size = img->height * img->width * sizeof(struct pixel_raw_t);
  struct pixel_raw_t *raw_data = malloc(raw_data_size);
  struct pixel_f32_t *normalised_data;
  memcpy(raw_data, (struct pixel_raw_t *)img->data, raw_data_size);
  free(img->data);
  if (!init_img_buffer(img, img->height, img->width, PIXEL_F32_T)) {
    return false;
  }

  normalised_data = img->data;
  printf("Normalising data from [%u,%u]\n", b, w);
  for (unsigned int i = 0; i < img->height * img->width; i++) {
    uint16_t raw_value;
    float norm_value;

    raw_value = raw_data[i].v;
    raw_value -= b;
    norm_value = (float)raw_value;
    norm_value *= (1.0f / (float)(w - b));
    normalised_data[i].v = norm_value;

    if (print_debugs) {
      printf("Normalised %u -> %f\n", raw_data[i].v, norm_value);
    }
  }

  free(raw_data);
  return true;
}

/*
 * CFA channel calculator - this isn't a pass, but is needed for working out
 * which channel we are in based on 1D data. We could use bitfields here, but
 * I haven't personally seen that much in the wild.
 */
struct cfa_descriptor {
  bool r;
  bool g;
  bool b;
};

// For now, assume the BGGR (hehe) pattern used by the RPi camera.
bool calculate_cfa_channel(struct image *img, unsigned int index,
                           struct cfa_descriptor *cfa_desc) {
  if (index >= img->height * img->width) {
    return false;
  }

  if (img->data_t == PIXEL_F32_T || img->data_t == PIXEL_RAW_T) {
    /* 1D data types */
    /*
     *      0  1  2  3  4  5  6  7  8  9 ...
     *
     * 0    B  G  B  G  B  G  B  G  B  G ...
     * 1    G  R  G  R  G  R  G  R  G  R ...
     * 2    B  G  B  G  B  G  B  G  B  G ...
     * 3    G  R  G  R  G  R  G  R  G  R ...
     *
     * So... my initial attempt at this:
     *
     * The beginning of the [G, R] pattern depends on the width of the image. Or
     * rather, we can ask ourselves "which pattern are we in?" based on i mod w
     * (the non-remainder part).
     *
     * Then, we use odd/even-ness of the remainder of i mod w to work out which
     * index of said pattern we need to use.
     *
     * I overcomplicated this the first time round and got caught up in the
     * img->width not being 0-indexed. Turns out you can just ignore that.
     *
     */
    unsigned int row = index / img->width;
    unsigned int column = index % img->width;
    bool row_even = row % 2 == 0;
    bool column_even = column % 2 == 0;

    cfa_desc->r = (!row_even && !column_even);
    cfa_desc->g = (row_even ^ column_even);
    cfa_desc->b = (row_even && column_even);

    return true;
  }
  printf("Can't calculate CFA channel for this data type!");
  return false;
}

/*
 * White Balance
 *
 * The next step is to calculate the white balance needed to properly light
 * the image to make it look more like the light we see in the real world.
 * This requires us to rescale the RGB values based on a pixel that we know
 * is 'white' in the real world.
 *
 * For this function, we use a struct pixel_f32_v3_t variable just to encode the
 * scales for each channel, not because it transforms the data type within the
 * image.
 *
 */
bool reduce_f32_v3_cfa(struct pixel_f32_v3_t *pixel,
                       struct cfa_descriptor *cfa) {
  if (!pixel || !cfa) {
    return false;
  }

  pixel->r *= (float)cfa->r;
  pixel->g *= (float)cfa->g;
  pixel->b *= (float)cfa->b;

  return true;
}

bool apply_wb(struct image *img, struct pixel_f32_v3_t *scales) {
  if (!img || !scales) {
    return false;
  }

  if (img->data_t != PIXEL_F32_T) {
    return false;
  }

  bool print_debugs = getenv(PASS_WB_DEBUG_STR) != NULL;

  for (unsigned int i = 0; i < img->height * img->width; i++) {
    // calculate which channel we need to use
    struct cfa_descriptor cfa = {0};
    struct pixel_f32_t *pixels = (struct pixel_f32_t *)img->data;
    float old_value = pixels[i].v;
    calculate_cfa_channel(img, i, &cfa);

    // There is possibly a more efficient way of handling this.
    if (cfa.r) {
      pixels[i].v *= scales->r;
    } else if (cfa.g) {
      // This usually doesn't change anything since G is used as the base point,
      // but adding this anyways just in case.
      pixels[i].v *= scales->g;
    } else if (cfa.b) {
      pixels[i].v *= scales->b;
    }

    if (print_debugs) {
      printf("Applied WB: %f -> %f\n", old_value, pixels[i].v);
    }
  }

  return true;
}

/*
 * Debayering
 *
 * The debayering pass will finally transform the normalised 32-bit float data
 * into three channels per pixel. The example BGGR CFA from above is copied
 * here:
 *
 *      0  1  2  3  4  5  6  7  8  9 ...
 *
 * 0    B  G  B  G  B  G  B  G  B  G ...
 * 1    G  R  G  R  G  R  G  R  G  R ...
 * 2    B  G  B  G  B  G  B  G  B  G ...
 * 3    G  R  G  R  G  R  G  R  G  R ...
 *
 */

bool calculate_x_y(unsigned int index, unsigned int width_in,
                   unsigned int *x_out, unsigned int *y_out) {
  if (!x_out || !y_out) {
    return false;
  }

  *x_out = index % width_in;
  *y_out = index / width_in;

  return true;
}

bool valid_coordinates(unsigned int width, unsigned int height, long x,
                       long y) {
  return (!(x < 0 || y < 0 || x >= width || y >= height));
}

void demosaic_cfa_r(struct image *img, long x, long y,
                    struct pixel_f32_v3_t *out) {
  struct pixel_f32_t *pixels = (struct pixel_f32_t *)img->data;

  int g_dirs[][2] = {
      {-1, 0},
      {1, 0},
      {0, -1},
      {0, 1},
  };

  int b_dirs[][2] = {
      {-1, -1},
      {1, 1},
      {-1, -1},
      {1, 1},
  };

  size_t g_count = sizeof(g_dirs) / sizeof(g_dirs[0]);
  float g_sum = 0.0f;
  int g_num = 0;
  for (int g_i = 0; g_i < g_count; g_i++) {
    long x_g = x + g_dirs[g_i][0];
    long y_g = y + g_dirs[g_i][1];

    if (valid_coordinates(img->width, img->height, x_g, y_g)) {
      g_sum += pixels[calculate_index(img, x_g, y_g)].v;
      g_num += 1;
    }
  }

  size_t b_count = sizeof(b_dirs) / sizeof(b_dirs[0]);
  float b_sum = 0.0f;
  int b_num = 0;
  for (int b_i = 0; b_i < g_count; b_i++) {
    long x_b = x + g_dirs[b_i][0];
    long y_b = y + g_dirs[b_i][1];

    if (valid_coordinates(img->width, img->height, x_b, y_b)) {
      b_sum += pixels[calculate_index(img, x_b, y_b)].v;
      b_num += 1;
    }
  }

  out->r = pixels[calculate_index(img, x, y)].v;
  out->g = (g_sum / (float)g_num);
  out->b = (b_sum / (float)b_num);
}

void demosaic_cfa_g(struct image *img, long x, long y,
                    struct pixel_f32_v3_t *out) {
  struct pixel_f32_t *pixels = (struct pixel_f32_t *)img->data;

  int even_dirs[][2] = {
      {-1, 0},
      {1, 0},
  };

  int odd_dirs[][2] = {
      {0, -1},
      {0, 1},
  };

  size_t count = sizeof(even_dirs) / sizeof(even_dirs[0]);
  float e_sum = 0.0f;
  float o_sum = 0.0f;
  int e_count = 0;
  int o_count = 0;

  for (int i = 0; i < count; i++) {
    long x_e = x + even_dirs[i][0];
    long y_e = y + even_dirs[i][1];

    if (valid_coordinates(img->width, img->height, x_e, y_e)) {
      e_sum += pixels[calculate_index(img, x_e, y_e)].v;
      e_count += 1;
    }

    long x_o = x + odd_dirs[i][0];
    long y_o = y + odd_dirs[i][1];

    if (valid_coordinates(img->width, img->height, x_o, y_o)) {
      o_sum += pixels[calculate_index(img, x_o, y_o)].v;
      o_count += 1;
    }
  }

  if (x % 2 == 0) {
    // R is odd_dirs, B is even_dirs
    out->r = o_sum / (float)o_count;
    out->g = e_sum / (float)e_count;
  } else {
    // B is odd_dirs, R is even_dirs
    out->r = e_sum / (float)e_count;
    out->g = o_sum / (float)o_count;
  }
  out->b = pixels[calculate_index(img, x, y)].v;
}

void demosaic_cfa_b(struct image *img, long x, long y,
                    struct pixel_f32_v3_t *out) {
  struct pixel_f32_t *pixels = (struct pixel_f32_t *)img->data;

  int g_dirs[][2] = {
      {-1, 0},
      {1, 0},
      {0, -1},
      {0, 1},
  };

  int r_dirs[][2] = {
      {-1, -1},
      {1, 1},
      {-1, -1},
      {1, 1},
  };

  size_t g_count = sizeof(g_dirs) / sizeof(g_dirs[0]);
  float g_sum = 0.0f;
  int g_num = 0;
  for (int g_i = 0; g_i < g_count; g_i++) {
    long x_g = x + g_dirs[g_i][0];
    long y_g = y + g_dirs[g_i][1];

    if (valid_coordinates(img->width, img->height, x_g, y_g)) {
      g_sum += pixels[calculate_index(img, x_g, y_g)].v;
      g_num += 1;
    }
  }

  size_t r_count = sizeof(r_dirs) / sizeof(r_dirs[0]);
  float r_sum = 0.0f;
  int r_num = 0;
  for (int r_i = 0; r_i < g_count; r_i++) {
    long x_r = x + g_dirs[r_i][0];
    long y_r = y + g_dirs[r_i][1];

    if (valid_coordinates(img->width, img->height, x_r, y_r)) {
      r_sum += pixels[calculate_index(img, x_r, y_r)].v;
      r_num += 1;
    }
  }

  out->r = (r_sum / (float)r_num);
  out->g = (g_sum / (float)g_num);
  out->b = pixels[calculate_index(img, x, y)].v;
}

// Bilinear Interpolation
bool apply_debayering_bi(struct image *img) {
  struct pixel_f32_v3_t *rgb_f_pixels;
  size_t rgb_f_pixels_size;
  struct pixel_f32_t *norm_pixels;
  unsigned int img_size;
  bool debug_print;

  if (!img) {
    return false;
  }

  if (img->data_t != PIXEL_F32_T) {
    return false;
  }

  img_size = img->height * img->width;
  norm_pixels = (struct pixel_f32_t *)img->data;
  rgb_f_pixels = malloc(img_size * sizeof(struct pixel_f32_v3_t));
  rgb_f_pixels_size = img->height * img->width * sizeof(struct pixel_f32_v3_t);
  debug_print = getenv(PASS_DEBAYER_DEBUG_STR) != NULL;

  if (!rgb_f_pixels) {
    printf("Error allocating space for RGB_F pixels!");
    return false;
  }

  for (unsigned int i = 0; i < img->height * img->width; i++) {
    // calculate which channel we need to use
    unsigned int i_x = 0;
    unsigned int i_y = 0;

    struct pixel_f32_v3_t *rgb_pixel = &rgb_f_pixels[i];
    struct cfa_descriptor cfa;

    calculate_x_y(i, img->width, &i_x, &i_y);
    calculate_cfa_channel(img, i, &cfa);

    /*
     * Each pixel will currently contain the data for a specific channel.
     * Once we know what channel this pixel corresponds to, we will need to
     * calculate the average of the nearest pixels which aren't of this channel.
     *
     * We can probably leverage our existing calculate_cfa_channel to write a
     * new function which will invert the values of cfa. Then, we need to
     * perform some 1D arithmetic to work out where the next pixel is going to
     * be.
     */

    if (cfa.r) {
      demosaic_cfa_r(img, (long)i_x, (long)i_y, rgb_pixel);
    } else if (cfa.g) {
      demosaic_cfa_g(img, (long)i_x, (long)i_y, rgb_pixel);
    } else if (cfa.b) {
      demosaic_cfa_b(img, (long)i_x, (long)i_y, rgb_pixel);
    }

    printf("Debayer: %f -> %f, %f, %f\n", norm_pixels[i].v, rgb_pixel->r,
           rgb_pixel->g, rgb_pixel->b);
  }

  // now we need to re-init our image buffer with the right
  // data.
  free(img->data);
  img->data = malloc(rgb_f_pixels_size);
  memcpy(img->data, rgb_f_pixels, rgb_f_pixels_size);
  free(rgb_f_pixels);
  img->data_t = PIXEL_F32_V3_T;

  return true;
}

/**
 * RPi Camera Data:
 * Scaling with darkness 4096, saturation 65535, and
 * multipliers 1.018250 1.000000 2.160646 1.00000
 */
int main() {
  // test_ppm_write();
  struct image img = (struct image){0};
  struct pixel_f32_v3_t wb_scales = (struct pixel_f32_v3_t){
      .r = 1.01825f,
      .g = 1.0f,
      .b = 2.160646f,
  };

  read_pgm("../res/rpi_test.pgm", &img);
  apply_normalisation(&img, 4096, 65535);
  apply_wb(&img, &wb_scales);
  apply_debayering_bi(&img);
  write_ppm("../res/rpi_test.ppm", &img);
  free_img_buffer(&img);
  return 0;
}
