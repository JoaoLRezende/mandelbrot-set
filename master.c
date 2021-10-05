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
  // In each execution, we start searching in the position that immediately
  // follows the last worker that we returned.
  static int worker_index = 0;
  while (worker_status_array[worker_index].is_busy) {
    worker_index = (worker_index + 1) % number_of_workers;
  }

  return worker_index;
}

// Find something for a worker to do.
static struct job get_next_job(int number_of_lines_already_commissioned) {
  return (struct job){.first_line = number_of_lines_already_commissioned,
                      .last_line =
                          number_of_lines_already_commissioned + LINES_PER_JOB};
}

// Send a job to a worker.
static void request_line_batch(struct job job, int worker) {
  const int target_worker_process_rank = worker + 1;
  MPI_Send(&job, sizeof(job), MPI_CHAR, target_worker_process_rank, 0,
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
  MPI_Get_count(&mpi_status, MPI_CHAR, &number_of_actually_received_pixels);
  assert(number_of_actually_received_pixels ==
         number_of_received_pixels * (int)sizeof(struct pixel));

  struct pixel *destination_buffer =
      output_img->arr + first_received_line * output_img->number_of_columns;
  MPI_Recv(destination_buffer,
           sizeof(output_img->arr[0]) * number_of_received_pixels, MPI_CHAR,
           MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpi_status);

  // TODO: we shouldn't be passing MPI_CHAR to MPI_Recv above, since we
  // aren't actually receiving chars. Do better. See see MPI functions that have
  // "type" in their name. Also see https://youtu.be/MNbMl1buV-I?t=985. Don't
  // forget to call MPI_Type_commit. And note that the number of elements to
  // receive and the mumber returned by MPI_Get_count will change: they will
  // become the number of received pixels. Remember to change too the data-type
  // argument passed to MPI_Get_count. We probably should call the
  // data-type-defining functions before branching into master- or
  // worker-specific code. The MPI_Datatype object that represents those objects
  // probably should be extern-declared in common.h, and defined in main.c
  // (edit: why not common.c?). Then be sure to change the other arguments that
  // are passed to MPI_Recv accordingly. Then change the workers' send code
  // accordingly. Also do the same in the function request_line_batch above, in
  // the worker's function receive_job_request, and in other functions that
  // exchange messages.

  // Check the MPI status object apropriately. Make sure no errors happened.

  fprintf(stderr, "Master has received the work done by process %d.\n",
          sending_worker_process_rank);

  return sending_worker_process_rank - 1;
}

/* Signal to workers that we're done. */
static void send_termination_message(int number_of_workers) {
  struct job termination_signal = {.first_line = -1, .last_line = -1};

  for (int worker = 1; worker <= number_of_workers; ++worker) {
    MPI_Send(&termination_signal, sizeof(termination_signal), MPI_CHAR, worker,
             0, MPI_COMM_WORLD);
  }
}

void do_master_stuff(int total_number_of_processes,
                     const char *output_filename) {
  struct image output_img = createImage(IMAGE_HEIGHT, IMAGE_WIDTH);
  // number_of_requested_lines holds the number of lines that we have already
  // asked a worker to compute. Since we commission lines sequentially, it also
  // is the offset of the first line that we still haven't asked a worker to
  // compute.
  int number_of_requested_lines = 0;
  int number_of_received_lines = 0;

  const int number_of_workers = total_number_of_processes - 1;
  struct worker_status *worker_statuses =
      calloc(number_of_workers, sizeof(*worker_statuses));
  int number_of_busy_workers = 0;

  fprintf(stderr, "Master is starting.\n");
  while (number_of_received_lines < output_img.number_of_lines) {
    // Commission as many lines as we can, until all workers are busy or there
    // are no more lines to commission.
    while (number_of_requested_lines < output_img.number_of_lines &&
           number_of_busy_workers < number_of_workers) {
      struct job job = get_next_job(number_of_requested_lines);
      int worker = get_idle_worker(worker_statuses, number_of_workers);
      request_line_batch(job, worker);
      { // temp, for debugging
        fprintf(stderr,
                "Master has asked process %d to compute lines %d to %d.\n",
                worker + 1, job.first_line, job.last_line);
      }
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

  free(worker_statuses);
  send_termination_message(number_of_workers);
  writePPM(output_filename, &output_img);
}
