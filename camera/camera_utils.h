#include <stdint.h>

#include <cstddef>

extern int rgb2hsl(int r, int g, int b);
extern bool filter(int hue, int desired, int threshold);

extern void imageToMask(bool mask[], const size_t maskLength, const uint8_t *imageData,
                 const size_t imageDataLength, const int desired,
                 const int threshold);
extern void maskToHistogram(uint8_t histogram[], const size_t histogramLength,
                            const bool mask[], const size_t maskLength);
extern int findMaxIndex(const uint8_t histogram[],
                        const size_t histogramLength);
