#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "master.h"
#include "worker.h"

int main(int argc, char **argv) {
  const char *output_filename = (argc > 1) ? argv[1] : DEFAULT_OUTPUT_FILENAME;
  const int image_height = (argc > 2) ? atoi(argv[2]) : DEFAULT_IMAGE_HEIGHT;
  const int image_width = (argc > 3) ? atoi(argv[3]) : DEFAULT_IMAGE_HEIGHT;

  MPI_Init(&argc, &argv);

  int this_process_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &this_process_rank);

  if (this_process_rank == 0) {
    do_master_stuff(output_filename, image_height, image_width);
  } else if (this_process_rank != 0) {
    do_worker_stuff(this_process_rank, image_height, image_width);
  }

  MPI_Finalize();
}
