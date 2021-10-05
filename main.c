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

  // Make sure that we can divide the lines among workers cleanly. We don't
  // know how to deal with leftover lines yet (TODO).
  if (IMAGE_HEIGHT % (total_number_of_processes - 1) != 0) {
    fputs("Can't divide the image's lines among workers cleanly. Please try a "
          "different number of processes.\n",
          stderr);
    abort();
  }

  if (this_process_rank == 0) {
    // TODO: why isn't do_master_stuff receiving the image's dimensions?
    do_master_stuff(total_number_of_processes, output_filename);

    { // temp, for debugging
      fprintf(stderr, "Master: we're finished! I'm about to call MPI_Abort.\n");
    }

    MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);

    { // temp, for debugging
      fprintf(stderr, "Aborted! Do I still exist?\n");
    }

  } else if (this_process_rank != 0) {
    do_worker_stuff(this_process_rank, IMAGE_HEIGHT, IMAGE_WIDTH);
  }

  MPI_Finalize();
}
