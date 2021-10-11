#include <stddef.h>
#include <sys/time.h>
#include <mpi.h>

long wtime() { /* funcao reaproveitada do nbodies_serial */
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec;
}

int min(int x, int y) {
  if (x < y) {
    return x;
  } else {
    return y;
  }
}

MPI_Datatype MPI_datatype_struct_job;
MPI_Datatype MPI_datatype_struct_pixel;

static void define_MPI_datatype_struct_job() {
  MPI_Type_contiguous(2, MPI_INT, &MPI_datatype_struct_job);
  MPI_Type_commit(&MPI_datatype_struct_job);
}

static void define_MPI_datatype_struct_pixel() {
  MPI_Type_contiguous(3, MPI_CHAR, &MPI_datatype_struct_pixel);
  MPI_Type_commit(&MPI_datatype_struct_pixel);
}

void define_MPI_datatypes() {
  define_MPI_datatype_struct_job();
  define_MPI_datatype_struct_pixel();
}
