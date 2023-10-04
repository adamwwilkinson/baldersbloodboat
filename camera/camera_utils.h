#include <stdint.h>

#include <cstddef>

#define NO_HUE 255;

typedef struct hsl {
  float h, s, l;
} HSL;

extern HSL rgb2hsl(float r, float g, float b);
extern bool filter(float hue, float desired, float threshold);

extern void imageToMask(bool mask[], const size_t maskLength,
                        const uint8_t *imageData, const size_t imageDataLength,
                        const float desired, const float threshold);
extern void maskToHistogram(uint8_t histogram[], const size_t histogramLength,
                            const bool mask[], const size_t maskLength);
extern int findMaxIndex(const uint8_t histogram[],
                        const size_t histogramLength);
