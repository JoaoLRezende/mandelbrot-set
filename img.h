
struct pixel {
  unsigned char r,g,b;
};

struct image {
 int x, y;
 struct pixel *arr;
};

void readPPM(const char *filename, struct image *pimage);
void writePPM(const char *filename, const struct image *pimage);
void cloneSizePPM(const struct image *src, struct image *dest);
void clonePPM(const struct image *src, struct image *dest);
void deletePPM(struct image *src);
void reservesizePPM(struct image *dest, const int x, const int y);
struct image createImage(const int x, const int y);
