#include "worker.h"

#include <assert.h>
#include <complex.h>
#include <mpi.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "constants.h"
#include "img.h"

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
    char c = (255 - n * iteration_factor);
    return (struct pixel){((c - 20) > 0) ? (c - 20)  : 0,
                          ((c + 40) < 255) ? (c + 40) : 255,
                          ((c + 60) < 255) ? (c + 60) : 255};
  }
}

static int process_part_of_image(struct pixel *restrict arr_out,
                                 const int total_image_height,
                                 const int total_image_width, struct job job) {
  const int number_of_lines = job.last_line - job.first_line;
  int number_of_threads_used;
#pragma omp parallel
#pragma omp single
  { number_of_threads_used = omp_get_num_threads(); }
#pragma omp for schedule(static, number_of_lines / omp_get_num_threads())
  for (int curr_line = job.first_line; curr_line < job.last_line; curr_line++) {
    struct pixel *first_pixel_of_line =
        &arr_out[(curr_line - job.first_line) * total_image_width];
    for (int curr_column = 0; curr_column < total_image_width; curr_column++) {
      struct pixel *curr_pixel = first_pixel_of_line + curr_column;
      *curr_pixel = getPixel((double)curr_column / (total_image_width - 1),
                             (double)curr_line / (total_image_height - 1));
    }
  }
  return number_of_threads_used;
}

static struct job receive_job_request() {
  struct job job;
  MPI_Status mpi_status;
  MPI_Recv(&job, 1, MPI_datatype_struct_job, 0, MPI_ANY_TAG, MPI_COMM_WORLD,
           &mpi_status);
  return job;
}

void do_worker_stuff(int this_process_rank, int total_image_height,
                     int total_image_width) {
  while (true) {
    struct job job = receive_job_request();

    if (job.first_line == -1) {
      return;
    }

    const long start = wtime();

    const int number_of_lines_to_process = job.last_line - job.first_line;
    const int number_of_pixels = number_of_lines_to_process * total_image_width;
    struct pixel *output_buffer =
        malloc(sizeof(struct pixel) * number_of_pixels);

    const int number_of_threads_used = process_part_of_image(
        output_buffer, total_image_height, total_image_width, job);

    fprintf(stderr,
            "Process %d processed lines %4d to %4d in %.4f seconds with %d "
            "threads.\n",
            this_process_rank, job.first_line, job.last_line,
            (wtime() - start) / 1000000.0, number_of_threads_used);

    MPI_Send(output_buffer, number_of_pixels, MPI_datatype_struct_pixel, 0, 0,
             MPI_COMM_WORLD);

    free(output_buffer);
  }
}
