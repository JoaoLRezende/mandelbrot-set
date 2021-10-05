#include "worker.h"

#include <assert.h>
#include <complex.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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
                                  const int total_image_width, struct job job) {
  for (int current_line = job.first_line; current_line < job.last_line;
       current_line++) {
    for (int current_column = 0; current_column < total_image_width;
         current_column++) {
      struct pixel *p =
          &arr_out[(current_line - job.first_line) * total_image_width +
                   current_column];
      *p = getPixel((double)current_column / (total_image_width - 1),
                    (double)current_line / (total_image_height - 1));
    }
  }
}

static struct job receive_job_request() {
  struct job job;
  MPI_Status mpi_status;
  MPI_Recv(&job, sizeof(job), MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD,
           &mpi_status);

  // Assert that we received as many bytes as we expected.
  int number_of_received_bytes;
  MPI_Get_count(&mpi_status, MPI_CHAR, &number_of_received_bytes);
  assert(number_of_received_bytes == sizeof(job));

  return job;
}

void do_worker_stuff(int this_process_rank, int total_image_height,
                     int total_image_width) {
  while (true) { // TODO: implement end condition
    struct job job = receive_job_request();

    long start = wtime();

    int number_of_lines_to_process = job.last_line - job.first_line;
    size_t output_buffer_size_in_bytes =
        sizeof(struct pixel) * number_of_lines_to_process * total_image_width;
    struct pixel *output_buffer = malloc(output_buffer_size_in_bytes);

    process_part_of_image(output_buffer, total_image_height, total_image_width,
                          job);

    MPI_Send(output_buffer, output_buffer_size_in_bytes, MPI_CHAR, 0, 0,
             MPI_COMM_WORLD);

    free(output_buffer);

    long end = wtime();
    fprintf(stderr, "Process %d processed lines %d to %d in %.6f seconds.\n",
            this_process_rank, job.first_line, job.last_line,
            (end - start) / 1000000.0);
  }
}
