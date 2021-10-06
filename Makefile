#args = -g  -Wall -Wextra -lm -fopenmp	# for debugging
args = -O3 -Wall -Wextra -lm -fopenmp	# for production

.PHONY : compile clean

compile : mandel

clean :
	rm mandel

mandel : src/main.c src/img.c src/img.h src/common.c src/common.h src/master.c src/master.h src/worker.c src/worker.h src/constants.h
	mpicc src/main.c src/img.c src/common.c src/master.c src/worker.c --output $@ $(args)


