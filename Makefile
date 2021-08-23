args = -g -Wall -Wextra -lm		# in production, we might want to use -O3 instead of -g


.PHONY : compile clean

compile : mandel

clean :
	rm mandel

mandel : main.c img.c img.h
	gcc main.c img.c -o $@ $(args)


