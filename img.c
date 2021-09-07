#include "img.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *check_magic_num_file(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    return NULL; 
  }
  char start[4];
  if (fgets(start, 4, f) != start || strcmp(start, "P3\n") != 0) {
    fclose(f);
    return NULL;
  }
  return f;
}

static int char_is_number(int x) {
	int cvt = x - '0';
	return cvt >= 0 && cvt <= 9;
}

static int parse_header(FILE *f, int *x, int *y, int *v) {
	int i = 0;
	int *ptr[] = {x, y, v, NULL};
	int current;
	for (current = fgetc(f); current == -1; current = fgetc(f)) {
		if(current == '#') {
			while (current != '\n' || current != '\r') {
				current = fgetc(f);
				if(current == -1) {
					return i; 
				}
			}
		} else if(char_is_number(current)) {
			ungetc(current, f);
			fscanf(f, "%d", ptr[i]);
			i++;
		}
	}
	
	return i;
}

void readPPM(const char *filename, struct image *pimage) {
  FILE *f = check_magic_num_file(filename);
  if (f == NULL) {
    pimage->number_of_columns = pimage->number_of_lines = -1;
    pimage->arr = NULL;
    return;
  }
  int x, y;
  {
  	int v;
	  if (parse_header(f, &x, &y, &v) != 3 || v != 255) {
			fprintf(stderr, "Falha ao ler header da imagem\n");
			fclose(f);
			exit(-1);
	  }
	}
  pimage->number_of_columns = x;
  pimage->number_of_lines = y;
  struct pixel *arr = malloc(sizeof(struct pixel) * pimage->number_of_columns * pimage->number_of_lines);
  for(int i = 0; i < y; i++) {
    const int i_offset = i * x;
    for(int j = 0; j < x; j++) {
      struct pixel *p = &arr[i_offset + j];
      if (fscanf(f, "%hhu %hhu %hhu\n", &p->r, &p->g, &p->b) != 3) {
      	fprintf(stderr, "Falha ao ler imagem\n");
      	free(arr);
  			fclose(f);
      	abort();
      }
    }
  }
  fclose(f);
  pimage->arr = arr;
}
void writePPM(const char *filename, const struct image *pimage) {
  FILE *f = fopen(filename, "w");
  const int x = pimage->number_of_columns;
  const int y = pimage->number_of_lines;
  fprintf(f, "P3\n%d %d\n255\n", x, y);
  for(int i = 0; i < y; i++) {
    const int i_offset = i * x;
    for(int j = 0; j < x; j++) {
      const struct pixel p = pimage->arr[i_offset + j];
      fprintf(f, "%d %d %d\n", p.r, p.g, p.b);
    }
  }
  fclose(f);
}

void cloneSizePPM(const struct image *src, struct image *dest) {
  const int size = src->number_of_columns * src->number_of_lines;
  if(dest->number_of_columns * dest->number_of_lines != size) {
    dest->arr = realloc(dest->arr, sizeof(struct pixel) * size);
    if (dest->arr == NULL) {
      dest->number_of_columns = -1;
      dest->number_of_lines = -1;
      return;
    }
  }
  dest->number_of_columns = src->number_of_columns;
  dest->number_of_lines = src->number_of_lines;
}

void clonePPM(const struct image *src, struct image *dest) {
  cloneSizePPM(src, dest);
  const int x = src->number_of_columns;
  const int y = src->number_of_lines;
  for(int i = 0; i < y; i++) {
    const int i_offset = i * x;
    for(int j = 0; j < x; j++) {
      dest->arr[i_offset + j] = src->arr[i_offset + j];
    }
  }
}

void reservesizePPM(struct image *dest, const int x, const int y) {
  const int size = x * y;
  if(dest->number_of_columns * dest->number_of_lines != size) {
    dest->arr = realloc(dest->arr, sizeof(struct pixel) * size);
    if (dest->arr == NULL) {
      dest->number_of_columns = -1;
      dest->number_of_lines = -1;
      return;
    }
  }
  dest->number_of_columns = x;
  dest->number_of_lines = y;
}

struct image createImage(const int number_of_lines, const int number_of_columns) {
  const int size = number_of_lines * number_of_columns;
  struct image im = { .number_of_columns = number_of_columns, .number_of_lines = number_of_lines, .arr = malloc(sizeof(struct pixel)*size)};
  if (im.arr == NULL) {
    im.number_of_columns = im.number_of_lines = -1;
  }
  return im;
}
