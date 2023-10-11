#include "camera_utils.h"

#include <Arduino.h>
#include <stdint.h>

// https://gist.github.com/ciembor/1494530
// Takes rgb values [0, 255]
// Returns HSL values from 0 to 1
// HSL rgb2hsl(float r, float g, float b) {
//   HSL result = {0, 0, 0};

//   r /= 255;
//   g /= 255;
//   b /= 255;

//   float colourMin = min(min(r, g), b);
//   float colourMax = max(max(r, g), b);

//   result.h = result.s = result.l = (colourMax + colourMin) / 2;

//   if (colourMax == colourMin) {
//     result.h = result.s = 0;  // achromatic
//   } else {
//     float d = colourMax - colourMin;
//     result.s = result.l > 0.5 ? d / (2 - colourMax - colourMin)
//                               : d / (colourMax + colourMin);

//     if (colourMax == r) {
//       result.h = (g - b) / d + (g < b ? 6 : 0);
//     } else if (colourMax == g) {
//       result.h = (b - r) / d + 2;
//     } else if (colourMax == b) {
//       result.h = (r - g) / d + 4;
//     }

//     result.h /= 6;
//   }

//   return result;
// }

// Values are akk frin 0 to 255
HSV rgb2hsv(RGB rgb) {
  HSV hsv;
  unsigned char rgbMin, rgbMax;

  rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b)
                         : (rgb.g < rgb.b ? rgb.g : rgb.b);
  rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b)
                         : (rgb.g > rgb.b ? rgb.g : rgb.b);

  hsv.v = rgbMax;
  if (hsv.v == 0) {
    hsv.h = 0;
    hsv.s = 0;
    return hsv;
  }

  hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
  if (hsv.s == 0) {
    hsv.h = 0;
    return hsv;
  }

  if (rgbMax == rgb.r)
    hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
  else if (rgbMax == rgb.g)
    hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
  else
    hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

  return hsv;
}

bool filter(int hue, int hueDesired, int hueThreshold, int sat,
            int satThreshold, int value, int valueThreshold) {
  int hueDiff = abs(hue - hueDesired);
  return (hueDiff < hueThreshold || 255 - hueDiff < hueThreshold) &&
         (sat > satThreshold) && (value > valueThreshold);
}

unsigned long imageToMask(bool mask[], const size_t maskLength,
                          const uint8_t *imageData,
                          const size_t imageDataLength, const int hueDesired,
                          const int hueThreshold, const int satThreshold,
                          const int valueThreshold) {
  size_t j = 0;
  size_t total = 0;
  for (size_t i = 0; i < imageDataLength - 2; i += 3) {
    if (j >= maskLength) {
      break;
    }

    uint8_t r = imageData[i + 2];
    uint8_t g = imageData[i + 1];
    uint8_t b = imageData[i + 0];
    RGB rgb = {r, g, b};

    HSV hsv = rgb2hsv(rgb);

    bool res = filter(hsv.h, hueDesired, hueThreshold, hsv.s, satThreshold,
                      hsv.v, valueThreshold);
    total += res;
    mask[j++] = res;
  }

  return total;
}

/**
 * @brief Given the mask of an image, convert it to a histogram
 *
 * @param histogram The histogram array to write to
 * @param histogramLength The length of the histogram array or width of the
 * image
 * @param mask The mask array to read from (1s and 0s)
 * @param maskLength The length of the mask array or width of the image
 */
void maskToHistogram(uint8_t histogram[], const size_t histogramLength,
                     const bool mask[], const size_t maskLength) {
  for (int i = 0; i < maskLength; i++) {
    if (mask[i] == 1) {
      histogram[i % histogramLength]++;
    }
  }
}

/**
 * @brief Given a histogram, find the index of the maximum value (the x
 * coordinate of the center of the detected object)
 *
 * @param histogram The histogram array to read from
 * @param histogramLength The length of the histogram array or width of the
 * image
 * @return int The index of the maximum value or x coordinate of the center of
 * the detected object
 */
int findMaxIndex(const uint8_t histogram[], const size_t histogramLength) {
  int maxIndex = 0;
  for (int i = 0; i < histogramLength; i++) {
    if (histogram[i] > histogram[maxIndex]) {
      maxIndex = i;
    }
  }
  return maxIndex;
}