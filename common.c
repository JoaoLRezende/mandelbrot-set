#include <stdio.h>
#include <stdlib.h>

#include "common.h"

struct line_range
get_line_range_processed_by_process(int process_rank,
                                    int total_number_of_processes,
                                    int total_number_of_lines) {
  int worker_count = total_number_of_processes - 1;
  int worker_num = process_rank - 1;

  // Make sure that we can divide the lines among workers cleanly. We don't know
  // how to deal with leftover lines yet.
  if (total_number_of_lines % worker_count != 0) {
    fputs("Can't divide the image's lines among workers cleanly. Please try a "
          "different number of processes.\n",
          stderr);
    abort();
  }

  int lines_per_worker = total_number_of_lines / worker_count;
  return (struct line_range){.first_line = worker_num * lines_per_worker,
                             .last_line = (worker_num + 1) * lines_per_worker};
}
