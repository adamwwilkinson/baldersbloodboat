#include <stdint.h>

#include <cstddef>

typedef struct rgb {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RGB;

// typedef struct hsl {
//   float h, s, l;
// } HSL;

typedef struct hsv {
  unsigned char h, s, v;
} HSV;

// HSL rgb2hsl(float r, float g, float b);
// extern bool filter(int hue, int desired, int threshold, int sat,
//                    int satThreshold);

extern HSV rgb2hsv(RGB rgb);

extern unsigned long imageToMask(bool mask[], const size_t maskLength,
                                 const uint8_t *imageData,
                                 const size_t imageDataLength,
                                 const int hueDesired, const int hueThreshold,
                                 const int satThreshold,
                                 const int valueThreshold);
extern void maskToHistogram(uint8_t histogram[], const size_t histogramLength,
                            const bool mask[], const size_t maskLength);
extern int findMaxIndex(const uint8_t histogram[],
                        const size_t histogramLength);
