#include <assert.h>
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

static void check_arguments(int argc, char **argv) {
  if (argc != 2 && argc != 1) {
    fprintf(stderr,
            "usage:\n\t%s <output_file = \"" DEFAULT_OUTPUT_FILENAME "\">\n",
            argv[0]);
    exit(-1);
  }
}

struct line_range {
  int first_line, last_line;
};

static struct line_range
get_line_range_processed_by_process(int process_rank,
                                    int total_number_of_processes,
                                    int total_number_of_lines) {
  int worker_count = total_number_of_processes - 1;
  int worker_num = process_rank - 1;

  // Make sure that we can divide the lines among workers cleanly. We don't know
  // how to deal with leftover lines yet.
  if (total_number_of_lines % worker_count != 0) {
    fputs("Can't divide the image's lines among workers cleanly. Please try a "
          "different number of proceses.\n",
          stderr);
    abort();
  }

  int lines_per_worker = total_number_of_lines / worker_count;
  return (struct line_range){.first_line = worker_num * lines_per_worker,
                             .last_line = (worker_num + 1) * lines_per_worker};
}

static void do_master_stuff(int total_number_of_processes,
                            const char *output_filename) {
  struct image output_img = createImage(600, 600);
  for (int worker_rank = 1; worker_rank < total_number_of_processes;
       worker_rank++) {
    struct line_range line_range_processed_by_this_process =
        get_line_range_processed_by_process(
            worker_rank, total_number_of_processes, output_img.number_of_lines);
    MPI_Recv(output_img.arr +
                 line_range_processed_by_this_process.first_line * output_img.number_of_columns,
             sizeof(output_img.arr[0]) * output_img.number_of_columns *
                 (line_range_processed_by_this_process.last_line -
                  line_range_processed_by_this_process.first_line),
             MPI_CHAR, worker_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fprintf(stderr, "Master has received the work done by process %d.\n",
            worker_rank);
  }

  writePPM(output_filename, &output_img);
}

static void process_part_of_image(struct image *restrict output_img,
                                  struct line_range line_range) {
  struct pixel *restrict arr_out = output_img->arr;

  const int height = output_img->number_of_lines;
  const int width = output_img->number_of_columns;
  for (int j = line_range.first_line; j < line_range.last_line; j++) {
    for (int i = 0; i < width; i++) {
      struct pixel *p = &arr_out[j * width + i];
      *p = getPixel((double)i / (width - 1), (double)j / (height - 1));
    }
  }
}

static void do_worker_stuff(int this_process_rank,
                            int total_number_of_processes) {
  long start = wtime();

  struct image output_img = createImage(600, 600);
  struct line_range line_range_to_process = get_line_range_processed_by_process(
      this_process_rank, total_number_of_processes, output_img.number_of_lines);

  process_part_of_image(&output_img, line_range_to_process);
  MPI_Send(
      output_img.arr + line_range_to_process.first_line * output_img.number_of_columns,
      sizeof(output_img.arr[0]) * output_img.number_of_columns *
          (line_range_to_process.last_line - line_range_to_process.first_line),
      MPI_CHAR, 0, 0, MPI_COMM_WORLD);

  long end = wtime();
  fprintf(stderr, "Process %d processed lines %d to %d in %.6f seconds.\n",
          this_process_rank, line_range_to_process.first_line,
          line_range_to_process.last_line, (end - start) / 1000000.0);
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
    do_worker_stuff(this_process_rank, total_number_of_processes);
  }

  MPI_Finalize();
}
