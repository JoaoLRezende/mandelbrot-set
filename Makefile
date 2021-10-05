args = -g -Wall -Wextra -lm	-fopenmp	# in production, we might want to use -O3 instead of -g


.PHONY : compile clean

compile : mandel

clean :
	rm mandel

mandel : main.c img.c img.h common.c common.h master.c master.h worker.c worker.h constants.h
	mpicc main.c img.c common.c master.c worker.c --output $@ $(args)


