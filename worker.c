#include "worker.h"

#include <complex.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <mpi.h>

#include "common.h"
#include "constants.h"
#include "img.h"

static long wtime() { /* funcao reaproveitada do nbodies_serial */
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec;
}

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

static void process_part_of_image(struct pixel *restrict arr_out,
                                  const int total_image_height,
                                  const int total_image_width,
                                  struct line_range line_range) {
  for (int current_line = line_range.first_line;
       current_line < line_range.last_line; current_line++) {
    for (int current_column = 0; current_column < total_image_width;
         current_column++) {
      struct pixel *p =
          &arr_out[(current_line - line_range.first_line) * total_image_width +
                   current_column];
      *p = getPixel((double)current_column / (total_image_width - 1),
                    (double)current_line / (total_image_height - 1));
    }
  }
}

void do_worker_stuff(int this_process_rank, int total_number_of_processes,
                     int total_image_height, int total_image_width) {
  long start = wtime();

  struct line_range line_range_to_process = get_line_range_processed_by_process(
      this_process_rank, total_number_of_processes, total_image_height);

  int number_of_lines_to_process =
      line_range_to_process.last_line - line_range_to_process.first_line;
  size_t output_buffer_size_in_bytes =
      sizeof(struct pixel) * number_of_lines_to_process * total_image_width;
  struct pixel *output_buffer = malloc(output_buffer_size_in_bytes);

  process_part_of_image(output_buffer, total_image_height, total_image_width,
                        line_range_to_process);

  MPI_Send(output_buffer, output_buffer_size_in_bytes, MPI_CHAR, 0, 0,
           MPI_COMM_WORLD);

  free(output_buffer);

  long end = wtime();
  fprintf(stderr, "Process %d processed lines %d to %d in %.6f seconds.\n",
          this_process_rank, line_range_to_process.first_line,
          line_range_to_process.last_line, (end - start) / 1000000.0);
}
