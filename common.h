#ifndef COMMON_H
#define COMMON_H

struct line_range {
  int first_line, last_line;
};

struct line_range
get_line_range_processed_by_process(int process_rank,
                                    int total_number_of_processes,
                                    int total_number_of_lines);

#endif
