#ifndef COMMON_H
#define COMMON_H

/* Represents a job. Instances of this type are sent from the master to workers.
 * An instance whose fields are both -1 means that the work is finished and the
 * worker should now terminate.
 */
struct job {
  int32_t first_line;
  int32_t last_line;
};

#endif
