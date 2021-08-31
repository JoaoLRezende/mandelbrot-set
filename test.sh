PROCESS_COUNT=2

mpiexec --use-hwthread-cpus -n $PROCESS_COUNT ./mandel
