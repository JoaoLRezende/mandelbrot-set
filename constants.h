#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DEFAULT_OUTPUT_FILENAME "mandel.ppm"
#define IMAGE_HEIGHT 600 // in pixels
#define IMAGE_WIDTH 600  // in pixels
#define LINES_PER_JOB 50 // TODO: this number is arbitrary. Experiment.

#define min_x (-2.5)
#define max_x 1.0
#define min_y (-1.0)
#define max_y 1
#define delta_x (max_x - min_x)
#define delta_y (max_y - min_y)

#define iterations 100
#define iteration_factor (255 / iterations)

#endif
