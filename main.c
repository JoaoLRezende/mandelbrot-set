
#include <complex.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "img.h"

static long wtime() { /* funcao reaproveitada do nbodies_serial */
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec;
}

static const double min_x = -2.5;
static const double max_x = 1;
static const double min_y = -1;
static const double max_y = 1;
static const double delta_x = max_x - min_x;
static const double delta_y = max_y - min_y;

static const int iterations = 100;
static const int iteration_factor = 255 / iterations;

static int num_iterations(float nx, float ny) {
  const double px = nx * delta_x + min_x;
  const double py = ny * delta_y + min_y;

  const double complex pz = px + py * I;
  // printf("%Zg\n", pz);

  double complex z = 0;

  for (int i = 0; i < iterations; i++) {
    z = z * z + pz;
    if (cabs(z) > 2) {
      return i;
    }
  }
  return -1;
}

static struct pixel getPixel(double nx, double ny) {
  int n = num_iterations(nx, ny);
  if (n == -1) {
    return (struct pixel){0, 0, 0};
  } else {
    unsigned char c = 255 - n * iteration_factor;
    return (struct pixel){c, c, c};
  }
}

static void process_image(struct image *restrict output_img) {
  struct pixel *restrict arr_out = output_img->arr;
  const int height = output_img->y;
  const int width = output_img->x;
  long start = wtime();
  for (int j = height - 1; j >= 0; j--) {
    for (int i = 0; i < width; i++) {
      struct pixel *p = &arr_out[j * width + i];
      *p = getPixel((double)i / (width - 1), (double)j / (height - 1));
    }
  }
  long end = wtime();
  printf("%.6f segundos \n", (end - start) / 1000000.0);
}

int main(int argc, char **argv) {
  if (argc != 2 && argc != 1) {
    fprintf(stderr, "Args: <output_file = \"mandel.ppm\">\n");
    exit(-1);
  }
  const char *output_filename = "mandel.ppm";
  if (argc > 1) {
    output_filename = argv[1]; /*repitam comigo, output_filename eh um ponteiro
                                  mutavel para uma string constante*/
  }
  struct image output_img = createImage(600, 600);
  process_image(&output_img);
  writePPM(output_filename, &output_img);
}
