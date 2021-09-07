#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "master.h"
#include "worker.h"

static void check_arguments(int argc, char **argv) {
  if (argc != 2 && argc != 1) {
    fprintf(stderr,
            "usage:\n\t%s <output_file = \"" DEFAULT_OUTPUT_FILENAME "\">\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  check_arguments(argc, argv);

  const char *output_filename = argc > 1 ? argv[1] : DEFAULT_OUTPUT_FILENAME;

  MPI_Init(&argc, &argv);

  int this_process_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &this_process_rank);
  int total_number_of_processes;
  MPI_Comm_size(MPI_COMM_WORLD, &total_number_of_processes);

  if (this_process_rank == 0) {
    do_master_stuff(total_number_of_processes, output_filename);
  } else if (this_process_rank != 0) {
    do_worker_stuff(this_process_rank, total_number_of_processes, IMAGE_HEIGHT,
                    IMAGE_WIDTH);
  }

  MPI_Finalize();
}
