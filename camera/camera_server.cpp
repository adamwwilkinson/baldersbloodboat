#include <Arduino.h>
#include <esp_now.h>

#include "camera.h"
#include "camera_utils.h"

const int hueDesired = 0;
const int hueThreshold = 25;
const int satThreshold = 70;
const int valueThreshold = 130;

int findRedDot(camera_fb_t *fb) {
  // make sure psram is available
  if (!psramFound()) {
    Serial.println("error: no psram available to store the RGB data");
    return 0;
  }
  unsigned long tTimer = millis();
  // allocate memory to store the rgb data (in psram, 3 bytes per pixel)
  Serial.println("Free psram before rgb data allocated = " +
                 String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) +
                 "K");
  void *ptrVal =
      NULL;  // create a pointer for memory location to store the data

  int IMAGE_WIDTH = fb->width;
  int IMAGE_HEIGHT = fb->height;
  int PIXEL_COUNT = IMAGE_WIDTH * IMAGE_HEIGHT;
  int ARRAY_LENGTH =
      PIXEL_COUNT * 3;  // calculate memory required to store the RGB data (i.e.
                        // number of pixels in the jpg image x 3)

  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH) {
    Serial.println("error: not enough free psram to store the rgb data");
    return 0;
  }
  ptrVal = heap_caps_malloc(
      ARRAY_LENGTH,
      MALLOC_CAP_SPIRAM);  // allocate memory space for the rgb data
  uint8_t *rgb = (uint8_t *)
      ptrVal;  // create the 'rgb' array pointer to the allocated memory space
  Serial.println("Free psram after rgb data allocated = " +
                 String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) +
                 "K");

  // convert the captured jpg image (fb) to rgb data (store in 'rgb' array)
  tTimer = millis();  // store time that image conversion process started
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (!jpeg_converted) {
    Serial.println("error: failed to convert image to RGB data");
    esp_camera_fb_return(fb);  // camera frame buffer
    heap_caps_free(ptrVal);    // rgb data
    return 0;
  }
  Serial.println("Conversion from jpg to RGB took " +
                 String(millis() - tTimer) +
                 " ms");  // report how long the conversion took

  // Print out the RGB data for the first 10 pixels
  // for (int i = 0; i < 10; i++) {
  //   Serial.println("RGB data for pixel " + String(i) + " = " +
  //                  String(rgb[i * 3 + 2]) + "," + String(rgb[i * 3 + 1]) +
  //                  "," + String(rgb[i * 3 + 0]));
  // }

  // get hues
  tTimer = millis();

  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < PIXEL_COUNT) {
    Serial.println("error: not enough free psram to store the mask");
    esp_camera_fb_return(fb);  // camera frame buffer
    heap_caps_free(ptrVal);    // rgb data
    return 0;
  }
  bool *mask = (bool *)heap_caps_malloc(PIXEL_COUNT, MALLOC_CAP_SPIRAM);

  unsigned long numPixels =
      imageToMask(mask, PIXEL_COUNT, rgb, ARRAY_LENGTH, hueDesired,
                  hueThreshold, satThreshold, valueThreshold);
  Serial.println("Conversion to mask took " + String(millis() - tTimer) +
                 " ms");
  if (numPixels < 512) {
    Serial.println("No red dot found");
    esp_camera_fb_return(fb);  // camera frame buffer
    heap_caps_free(ptrVal);    // rgb data
    heap_caps_free(mask);
    return 0;
  }

  // get histogram
  uint8_t histogram[IMAGE_WIDTH] = {0};
  maskToHistogram(histogram, IMAGE_WIDTH, mask, PIXEL_COUNT);

  // get max
  int maxIndex = findMaxIndex(histogram, IMAGE_WIDTH);

  // get percentage from center
  int percentage = (maxIndex - IMAGE_WIDTH / 2) * 100 / (IMAGE_WIDTH / 2);

  Serial.println("Desired hue: " + String(hueDesired));
  Serial.println("Threshold: " + String(hueThreshold));
  Serial.println("Max index (Detected x): " + String(maxIndex));
  Serial.println("Percentage from center: " + String(percentage));

  // finished with the data so free up the memory space used in psram
  esp_camera_fb_return(fb);  // camera frame buffer
  heap_caps_free(ptrVal);    // rgb data
  heap_caps_free(mask);
  return maxIndex;
}
