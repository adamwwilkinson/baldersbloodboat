// this is is just so other files can read these global variables, everything is
// defined in camera.ino
#include <stdint.h>

#include "esp_camera.h"

extern const char *stitle;    // title of this sketch
extern const char *sversion;  // Sketch version

// Camera related
extern bool
    flashRequired;  // If flash to be used when capturing image (1 = yes)
extern framesize_t FRAME_SIZE_IMAGE;  // Resolution
#define PIXFORMAT PIXFORMAT_JPEG;     // TODO: investigate RGB888
extern int
    cameraImageExposure;  // Camera exposure (0 - 1200)   If gain and exposure
                          // both set to zero then auto adjust is enabled
extern int cameraImageGain;        // Image gain (0 - 30)
extern int cameraImageBrightness;  // Image brightness (-2 to +2)

extern const int timeBetweenStatus;  // speed of flashing system running ok
                                     // status light (milliseconds)

extern const int indicatorLED;  // onboard small LED pin (33)

// Bright LED (Flash)
extern const int brightLED;       // onboard Illumination/flash LED pin (4)
extern int brightLEDbrightness;   // initial brightness (0 - 255)
extern const int ledFreq;         // PWM settings
extern const int ledChannel;      // camera uses timer1
extern const int ledRresolution;  // resolution (8 = from 0 to 255)

extern const int iopinA;  // general io pin 13
extern const int iopinB;  // general io pin 12 (must not be high at boot)

extern const int serialSpeed;  // Serial data speed to use

extern uint32_t
    lastStatus;  // last time status light changed status (to flash all ok led)
extern String imageResDetails;  // image resolution info

// https://randomnerdtutorials.com/esp32-cam-camera-pin-gpios/
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32   // power to camera (on/off)
#define RESET_GPIO_NUM -1  // -1 = not used
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26  // i2c sda
#define SIOC_GPIO_NUM 27  // i2c scl
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25  // vsync_pin
#define HREF_GPIO_NUM 23   // href_pin
#define PCLK_GPIO_NUM 22   // pixel_clock_pin

#define ERROR_CATASTROPHIC -999
#define ERROR_NOTHING_DETECTED -998