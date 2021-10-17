#include "master.h"

#include <assert.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "constants.h"
#include "img.h"

struct worker_status {
  bool is_busy;
  struct job current_job; // valid only if is_busy is true
};

static int get_idle_worker(struct worker_status *worker_status_array,
                           int number_of_workers) {
  // In each call, we start searching in the position that immediately
  // follows the last worker that we returned.
  static int worker_index = 0;
  while (worker_status_array[worker_index].is_busy) {
    worker_index = (worker_index + 1) % number_of_workers;
  }

  return worker_index;
}

// Find something for a worker to do.
static struct job get_next_job(int number_of_lines_already_commissioned,
                               int total_number_of_lines, int lines_per_job) {
  const int number_of_lines_not_yet_commissioned =
      total_number_of_lines - number_of_lines_already_commissioned;

  const int number_of_lines_to_request =
      min(lines_per_job, number_of_lines_not_yet_commissioned);

  return (struct job){.first_line = number_of_lines_already_commissioned,
                      .last_line = number_of_lines_already_commissioned +
                                   number_of_lines_to_request};
}

// Send a job to a worker.
static void request_line_batch(struct job job, int worker) {
  const int target_worker_process_rank = worker + 1;
  MPI_Send(&job, 1, MPI_datatype_struct_job, target_worker_process_rank, 0,
           MPI_COMM_WORLD);
}

/* [blocking] Receive from a worker a commissioned line batch. Store it
 * in the appropriate part of the image buffer.
 * Returns the index of the worker that sent the received line batch.
 */
static int receive_job_result(struct image *output_img,
                              const struct worker_status *worker_status_array) {
  MPI_Status mpi_status;
  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpi_status);

  int sending_worker_process_rank = mpi_status.MPI_SOURCE;
  const struct worker_status *sending_worker_status =
      &worker_status_array[sending_worker_process_rank - 1];
  assert(sending_worker_status->is_busy);

  int first_received_line = sending_worker_status->current_job.first_line;
  int number_of_received_lines =
      sending_worker_status->current_job.last_line - first_received_line;
  int number_of_received_pixels =
      number_of_received_lines * output_img->number_of_columns;

  // Make sure that we're receiving as many bytes as we expect.
  int number_of_actually_received_pixels;
  MPI_Get_count(&mpi_status, MPI_datatype_struct_pixel,
                &number_of_actually_received_pixels);
  assert(number_of_actually_received_pixels == number_of_received_pixels);

  struct pixel *destination_buffer =
      output_img->arr + first_received_line * output_img->number_of_columns;
  MPI_Recv(destination_buffer, number_of_received_pixels,
           MPI_datatype_struct_pixel, MPI_ANY_SOURCE, MPI_ANY_TAG,
           MPI_COMM_WORLD, &mpi_status);

  return sending_worker_process_rank - 1;
}

/* Signal to workers that we're done. */
static void send_termination_message(int number_of_workers) {
  struct job termination_signal = {.first_line = -1, .last_line = -1};

  for (int worker = 1; worker <= number_of_workers; ++worker) {
    MPI_Send(&termination_signal, 1, MPI_datatype_struct_job, worker, 0,
             MPI_COMM_WORLD);
  }
}

void do_master_stuff(const char *output_filename, int image_height,
                     int image_width, int lines_per_job) {
  struct image output_img = createImage(image_height, image_width);
  // number_of_requested_lines holds the number of lines that we have already
  // asked a worker to compute. Since we commission lines sequentially, it also
  // is the offset of the first line that we still haven't asked a worker to
  // compute.
  int number_of_requested_lines = 0;
  int number_of_received_lines = 0;

  int total_number_of_processes;
  MPI_Comm_size(MPI_COMM_WORLD, &total_number_of_processes);
  const int number_of_workers = total_number_of_processes - 1;
  struct worker_status *worker_statuses =
      calloc(number_of_workers, sizeof(*worker_statuses));
  int number_of_busy_workers = 0;

  const long int start_time = wtime();
  while (number_of_received_lines < output_img.number_of_lines) {
    // Commission as many lines as we can, until all workers are busy or there
    // are no more lines to commission.
    while (number_of_requested_lines < output_img.number_of_lines &&
           number_of_busy_workers < number_of_workers) {
      struct job job = get_next_job(number_of_requested_lines,
                                    output_img.number_of_lines, lines_per_job);
      int worker = get_idle_worker(worker_statuses, number_of_workers);
      request_line_batch(job, worker);
      worker_statuses[worker].is_busy = true;
      worker_statuses[worker].current_job = job;
      number_of_requested_lines += job.last_line - job.first_line;
      number_of_busy_workers += 1;
    }

    // Receive a line batch from a worker.
    int worker = receive_job_result(&output_img, worker_statuses);
    number_of_received_lines += worker_statuses[worker].current_job.last_line -
                                worker_statuses[worker].current_job.first_line;
    worker_statuses[worker].is_busy = false;
    number_of_busy_workers -= 1;
  }

  fprintf(stderr, "We're done! Time taken: %.5f seconds.\n",
          (wtime() - start_time) / 1000000.0);

  free(worker_statuses);
  send_termination_message(number_of_workers);
  writePPM(output_filename, &output_img);
}
