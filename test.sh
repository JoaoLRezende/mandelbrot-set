PROCESS_COUNT=3                # total number of MPI nodes, including master
export OMP_NUM_THREADS=2       # threads per worker node

mpiexec --use-hwthread-cpus -n $PROCESS_COUNT ./mandel
