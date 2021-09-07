#include "master.h"

#include <mpi.h>
#include <stdio.h>

#include "common.h"
#include "constants.h"
#include "img.h"

void do_master_stuff(int total_number_of_processes,
                     const char *output_filename) {
  struct image output_img = createImage(IMAGE_HEIGHT, IMAGE_WIDTH);

  for (int worker_rank = 1; worker_rank < total_number_of_processes;
       worker_rank++) {
    struct line_range line_range_processed_by_this_process =
        get_line_range_processed_by_process(
            worker_rank, total_number_of_processes, output_img.number_of_lines);

    MPI_Recv(output_img.arr + line_range_processed_by_this_process.first_line *
                                  output_img.number_of_columns,
             sizeof(output_img.arr[0]) * output_img.number_of_columns *
                 (line_range_processed_by_this_process.last_line -
                  line_range_processed_by_this_process.first_line),
             MPI_CHAR, worker_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    fprintf(stderr, "Master has received the work done by process %d.\n",
            worker_rank);
  }

  writePPM(output_filename, &output_img);
}
