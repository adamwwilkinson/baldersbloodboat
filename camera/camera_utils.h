#include <stdint.h>

#define NO_HUE 255;

typedef struct hsl {
  float h, s, l;
} HSL;

extern HSL rgb2hsl(float r, float g, float b);
extern bool filter(float hue, float desired, float threshold);

