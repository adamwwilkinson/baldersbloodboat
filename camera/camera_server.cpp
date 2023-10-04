#include <esp_now.h>

#include "camera.h"
#include "camera_utils.h"

float readRGBImage() {
  static float desired = 135 / 255.0;
  static float threshold = 10 / 255.0;

  // make sure psram is available
  if (!psramFound()) {
    Serial.println("error: no psram available to store the RGB data");
    return 0;
  }

  // capture a live image from camera (as a jpg)
  camera_fb_t *fb = NULL;
  unsigned long tTimer = millis(); // store time that image capture started
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("error: failed to capture image from camera");
    return 0;
  } else {
    Serial.println("JPG image capture took " + String(millis() - tTimer) +
                   " ms"); // report time it took to capture an image
    Serial.println("Image resolution=" + String(fb->width) + "x" +
                   String(fb->height));
    Serial.println("Image size=" + String(fb->len) + " bytes");
    Serial.println("Image format=" + String(fb->format));
    Serial.println("Free memory=" + String(ESP.getFreeHeap()) + " bytes");
  }

  // allocate memory to store the rgb data (in psram, 3 bytes per pixel)
  Serial.println("Free psram before rgb data allocated = " +
                 String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) +
                 "K");
  void *ptrVal = NULL; // create a pointer for memory location to store the data

  uint32_t IMAGE_WIDTH = fb->width;
  uint32_t IMAGE_HEIGHT = fb->height;
  uint32_t PIXEL_COUNT = IMAGE_WIDTH * IMAGE_HEIGHT;
  uint32_t ARRAY_LENGTH =
      PIXEL_COUNT * 3; // calculate memory required to store the RGB data (i.e.
                       // number of pixels in the jpg image x 3)

  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH) {
    Serial.println("error: not enough free psram to store the rgb data");
    return 0;
  }
  ptrVal = heap_caps_malloc(
      ARRAY_LENGTH,
      MALLOC_CAP_SPIRAM); // allocate memory space for the rgb data
  uint8_t *rgb = (uint8_t *)
      ptrVal; // create the 'rgb' array pointer to the allocated memory space
  Serial.println("Free psram after rgb data allocated = " +
                 String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) +
                 "K");

  // convert the captured jpg image (fb) to rgb data (store in 'rgb' array)
  tTimer = millis(); // store time that image conversion process started
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (!jpeg_converted) {
    Serial.println("error: failed to convert image to RGB data");
    return 0;
  }
  Serial.println("Conversion from jpg to RGB took " +
                 String(millis() - tTimer) +
                 " ms"); // report how long the conversion took

  // get hues
  bool mask[PIXEL_COUNT] = {0};
  imageToMask(mask, PIXEL_COUNT, rgb, ARRAY_LENGTH, desired, threshold);

  // get histogram
  uint8_t histogram[IMAGE_WIDTH] = {0};
  maskToHistogram(histogram, IMAGE_WIDTH, mask, PIXEL_COUNT);

  // get max
  int maxIndex = findMaxIndex(histogram, IMAGE_WIDTH);

  // get percentage from center
  float percentage = (float)(maxIndex - (IMAGE_WIDTH / 2)) / (float)IMAGE_WIDTH;

  Serial.println("Desired hue: " + String(desired));
  Serial.println("Threshold: " + String(threshold));
  Serial.println("Max index (Detected x): " + String(maxIndex));
  Serial.println("Percentage from center: " + String(percentage));

  // finished with the data so free up the memory space used in psram
  esp_camera_fb_return(fb); // camera frame buffer
  heap_caps_free(ptrVal);   // rgb data
  return percentage;
}