process_count=3                 # total number of MPI nodes, including master
lines_per_job=100
export OMP_NUM_THREADS=2        # threads per worker node

output_image="output/output.ppm"
image_height="3000"             # in pixels
image_width="3000"              # in pixels

cd "$(dirname "$0")"

mpiexec --use-hwthread-cpus -n $process_count \
    mandelbrot-set-generator ${output_image} ${image_height} ${image_width} ${lines_per_job}
