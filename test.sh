PROCESS_COUNT=4

mpiexec --use-hwthread-cpus -n $PROCESS_COUNT ./mandel
