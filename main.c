
#include <complex.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "img.h"

#define DEFAULT_OUTPUT_FILENAME "mandel.ppm"

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

static void check_arguments(int argc, char **argv) {
  if (argc != 2 && argc != 1) {
    fprintf(stderr,
            "usage:\n\t%s <output_file = \"" DEFAULT_OUTPUT_FILENAME "\">\n",
            argv[0]);
    exit(-1);
  }
}

static void do_master_stuff(int total_number_of_processes,
                            const char *output_filename) {
  const int OTHER_PROCESS_RANK = 1;
  struct image output_img = createImage(600, 600);
  MPI_Recv(output_img.arr, get_size_of_image_buffer(&output_img), MPI_CHAR,
           OTHER_PROCESS_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  writePPM(output_filename, &output_img);
}

static void do_worker_stuff() {
  struct image output_img = createImage(600, 600);
  process_image(&output_img);
  MPI_Send(output_img.arr, get_size_of_image_buffer(&output_img), MPI_CHAR, 0,
           0, MPI_COMM_WORLD);
}

int main(int argc, char **argv) {
  check_arguments(argc, argv);

  const char *output_filename = DEFAULT_OUTPUT_FILENAME;
  if (argc > 1) {
    output_filename = argv[1];
  }

  MPI_Init(&argc, &argv);

  int this_process_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &this_process_rank);
  int total_number_of_processes;
  MPI_Comm_size(MPI_COMM_WORLD, &total_number_of_processes);

  if (this_process_rank == 0) {
    do_master_stuff(total_number_of_processes, output_filename);
  } else if (this_process_rank != 0) {
    do_worker_stuff();
  }

  MPI_Finalize();
}
