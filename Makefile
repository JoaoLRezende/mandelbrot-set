args = -g -Wall -Wextra -lm		# in production, we might want to use -O3 instead of -g


.PHONY : compile clean

compile : mandel

clean :
	rm mandel

mandel : main.c img.c img.h
	mpicc main.c img.c --output $@ $(args)


