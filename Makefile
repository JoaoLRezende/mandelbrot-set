args = -O2 -Wall -Wextra -lm


.PHONY : compile clean

compile : mandel

clean :
	rm mandel

mandel : main.c img.c img.h
	gcc main.c img.c -o $@ $(args)


