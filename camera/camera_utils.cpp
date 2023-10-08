#include <Arduino.h>
#include <stdint.h>
#include "camera_utils.h"

// https://gist.github.com/ciembor/1494530
// Takes rgb values [0, 255]
// Returns hsl struct with values [0, 1]
HSL rgb2hsl(float r, float g, float b) {
  HSL result;
  
  r /= 255;
  g /= 255;
  b /= 255;
  
  float colourMax = max(max(r,g),b);
  float colourMin = min(min(r,g),b);
  
  result.h = result.s = result.l = (colourMax + colourMin) / 2;

  if (colourMax == colourMin) {
    result.h = result.s = 0;
  }
  else {
    float d = colourMax - colourMin;
    result.s = (result.l > 0.5) ? d / (2 - colourMax - colourMin) : d / (colourMax + colourMin);
    
    if (colourMax == r) {
      result.h = (g - b) / d + (g < b ? 6 : 0);
    }
    else if (colourMax == g) {
      result.h = (b - r) / d + 2;
    }
    else if (colourMax == b) {
      result.h = (r - g) / d + 4;
    }
    
    result.h /= 6;
  }

  return result;
}

bool filter(float hue, float desired, float threshold) {
  float diff = abs(hue - desired);
  return (diff < threshold || 1-diff < threshold) && (hue != 0);
}

/**
 * @brief Given an rgb image, convert it to a mask of 1s and 0s
 * 
 * @param mask Pointer to the mask array to write to (1s and 0s)
 * @param maskLength Length of the mask array
 * @param imageData Pointer to the image data array. The array should be in the format [b, g, r, b, g, r, ...]
 * @param imageDataLength Length of the image data array
 * @param desired The desired hue to filter for
 * @param threshold The threshold for the hue filter
 */
void imageToMask(bool mask[], const size_t maskLength, const uint8_t *imageData, const size_t imageDataLength, const float desired, const float threshold) {
  int j = 0;
  for (int i = 0; i < imageDataLength - 2; i += 3) {
    if (j >= maskLength) {
      break;
    }

    uint8_t r = imageData[i+2];
    uint8_t g = imageData[i+1];
    uint8_t b = imageData[i+0];

    HSL hsl = rgb2hsl(r, g, b);

    // only use the hue for now
    if (filter(hsl.h, desired, threshold)) {
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
 * @param histogramLength The length of the histogram array or width of the image
 * @param mask The mask array to read from (1s and 0s)
 * @param maskLength The length of the mask array or width of the image
 */
void maskToHistogram(uint8_t histogram[], const size_t histogramLength, const bool mask[], const size_t maskLength) {
  for (int i = 0; i < maskLength; i++) {
    if (mask[i] == 1) {
      histogram[i % histogramLength]++;
    }
  }
}

/**
 * @brief Given a histogram, find the index of the maximum value (the x coordinate of the center of the detected object)
 * 
 * @param histogram The histogram array to read from
 * @param histogramLength The length of the histogram array or width of the image
 * @return int The index of the maximum value or x coordinate of the center of the detected object
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