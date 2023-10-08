#include "camera_utils.h"

#include <Arduino.h>
#include <stdint.h>

// https://gist.github.com/ciembor/1494530
// Takes rgb values [0, 255]
// Returns hue integer from [0, 360]
int rgb2hsl(int r, int g, int b) {
  float colourMin = min(min(r, g), b);
  float colourMax = max(max(r, g), b);

  if (colourMin == colourMax) {
    return 0;
  }

  float hue = 0;
  if (colourMax == r) {
    hue = (g - b) / (colourMax - colourMin);
  } else if (colourMax == g) {
    hue = 2.0 + (b - r) / (colourMax - colourMin);

  } else {
    hue = 4.0 + (r - g) / (colourMax - colourMin);
  }

  hue = hue * 60;
  if (hue < 0) hue = hue + 360;
  return round(hue);
}

bool filter(int hue, int desired, int threshold) {
  int diff = abs(hue - desired);
  return (diff < threshold);
}

/**
 * @brief Given an rgb image, convert it to a mask of 1s and 0s
 *
 * @param mask Pointer to the mask array to write to (1s and 0s)
 * @param maskLength Length of the mask array
 * @param imageData Pointer to the image data array. The array should be in the
 * format [b, g, r, b, g, r, ...]
 * @param imageDataLength Length of the image data array
 * @param desired The desired hue to filter for
 * @param threshold The threshold for the hue filter
 */
void imageToMask(bool mask[], const size_t maskLength, const uint8_t *imageData,
                 const size_t imageDataLength, const int desired,
                 const int threshold) {
  size_t j = 0;
  for (size_t i = 0; i < imageDataLength - 2; i += 3) {
    if (j >= maskLength) {
      break;
    }

    uint8_t r = imageData[i + 2];
    uint8_t g = imageData[i + 1];
    uint8_t b = imageData[i + 0];

    int hue = rgb2hsl(r, g, b);

    if (filter(hue, desired, threshold)) {
      mask[j++] = 1;
    } else {
      mask[j++] = 0;
    }
  }
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