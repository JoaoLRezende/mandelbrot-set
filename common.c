#include <stddef.h>
#include <sys/time.h>

long wtime() { /* funcao reaproveitada do nbodies_serial */
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec;
}
