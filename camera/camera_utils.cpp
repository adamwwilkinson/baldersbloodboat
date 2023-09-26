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