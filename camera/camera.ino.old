// Reference:
// https://github.com/alanesq/esp32cam-demo/blob/master/ESP32cam-demo.ino

#if !defined ESP32
#error This sketch is only for an ESP32Cam module
#endif

#include <WiFi.h>
#include <esp_now.h>

#include "camera.h"
#include "camera_server.h"
#include "driver/ledc.h"
#include "esp_camera.h"

// sd-card
#include <FS.h>  // gives file access
#include <SPI.h>

#include "SD_MMC.h"  // sd card - see https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/
#define SD_CS 5      // sd chip select pin = 5

typedef struct struct_message {
  int a;
} struct_message;

const uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0xFC, 0x8A, 0x48};
esp_now_peer_info_t peerInfo;

const char *stitle = "ESP32Cam";  // title of this sketch
const char *sversion = "1.0";     // Sketch version

// Camera related
bool flashRequired = 1;  // If flash to be used when capturing image (1 = yes)
framesize_t FRAME_SIZE_IMAGE = FRAMESIZE_SVGA;  // Resolution
#define PIXFORMAT PIXFORMAT_JPEG;               // TODO: investigate RGB888
int cameraImageExposure =
    0;  // Camera exposure (0 - 1200)   If gain and exposure both set to zero
        // then auto adjust is enabled
int cameraImageGain = 0;        // Image gain (0 - 30)
int cameraImageBrightness = 0;  // Image brightness (-2 to +2)

const int timeBetweenStatus =
    600;  // speed of flashing system running ok status light (milliseconds)

const int indicatorLED = 33;  // onboard small LED pin (33)

// Bright LED (Flash)
const int brightLED = 4;       // onboard Illumination/flash LED pin (4)
int brightLEDbrightness = 0;   // initial brightness (0 - 255)
const int ledFreq = 5000;      // PWM settings
const int ledChannel = 15;     // camera uses timer1
const int ledRresolution = 8;  // resolution (8 = from 0 to 255)

const int iopinA = 13;  // general io pin 13
const int iopinB = 12;  // general io pin 12 (must not be high at boot)

const int serialSpeed = 115200;  // Serial data speed to use

uint32_t lastStatus =
    millis();  // last time status light changed status (to flash all ok led)
String imageResDetails = "Unknown";  // image resolution info
bool sdcardPresent;                  // flag if an sd card is detected
int imageCounter;                    // image file name on sd card counter

// Random utility functions that I couldn't figure out how to separate
void flashLED(int reps) {
  for (int x = 0; x < reps; x++) {
    digitalWrite(indicatorLED, LOW);
    delay(1000);
    digitalWrite(indicatorLED, HIGH);
    delay(500);
  }
}

void brightLed(byte ledBrightness) {
  brightLEDbrightness = ledBrightness;   // store setting
  ledcWrite(ledChannel, ledBrightness);  // change LED brightness (0 - 255)
  Serial.println("Brightness changed to " + String(ledBrightness));
}

void setupFlashPWM() {
  ledcSetup(ledChannel, ledFreq, ledRresolution);
  ledcAttachPin(brightLED, ledChannel);
  brightLed(brightLEDbrightness);
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

void setup() {
  Serial.begin(serialSpeed);  // Start serial communication
  // Serial.setDebugOutput(true);

  Serial.println("\n\n\n");  // line feeds
  Serial.println("-----------------------------------");
  Serial.printf("Starting - %s - %s \n", stitle, sversion);
  Serial.println("-----------------------------------");

  // small indicator led on rear of esp32cam board
  pinMode(indicatorLED, OUTPUT);
  digitalWrite(indicatorLED, HIGH);

  // Connect to wifi
  digitalWrite(indicatorLED, LOW);  // small indicator led on

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.macAddress());

  digitalWrite(indicatorLED, HIGH);  // small indicator led off

  // set up camera
  Serial.print(("\nInitialising camera: "));
  if (initialiseCamera()) {
    Serial.println("Camera Initialised OK");
  } else {
    Serial.println("Camera Initailised failed");
  }

  // define i/o pins
  pinMode(indicatorLED,
          OUTPUT);  // defined again as sd card config can reset it
  digitalWrite(indicatorLED, HIGH);  // led off = High
  pinMode(iopinA,
          INPUT);  // pin 13 - free io pin, can be used for input or output
  pinMode(iopinB, OUTPUT);  // pin 12 - free io pin, can be used for input or
                            // output (must not be high at boot)

  setupFlashPWM();  // configure PWM for the illumination LED

  // startup complete
  Serial.println("\nStarted...");
  // flashLED(2);    // flash the onboard indicator led
  // brightLed(64);  // change bright LED
  // delay(200);
  // brightLed(0);  // change bright LED
}

// ! Here is the money
void loop() {
  // flash status LED to show sketch is running ok
  if ((unsigned long)(millis() - lastStatus) >= timeBetweenStatus) {
    lastStatus = millis();  // reset timer
    digitalWrite(indicatorLED,
                 !digitalRead(indicatorLED));  // flip indicator led status
  }

  int percentage = readRGBImage();
  struct_message data = {percentage};
  esp_err_t result =
      esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(struct_message));

  usleep(1000000);  // sleep for 10 second
}

bool initialiseCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz =
      20000000;  // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  config.pixel_format =
      PIXFORMAT;  // Options =  YUV422, GRAYSCALE, RGB565, JPEG, RGB888
  config.frame_size =
      FRAME_SIZE_IMAGE;  // Image sizes: 160x120 (QQVGA), 128x160 (QQVGA2),
                         // 176x144 (QCIF), 240x176 (HQVGA), 320x240 (QVGA),
                         //              400x296 (CIF), 640x480 (VGA, default),
                         //              800x600 (SVGA), 1024x768 (XGA),
                         //              1280x1024 (SXGA), 1600x1200 (UXGA)
  config.jpeg_quality =
      63;  // 0-63 lower number means higher quality (can cause failed image
           // capture if set too low at higher resolutions)
  config.fb_count =
      1;  // if more than one, i2s runs in continuous mode. Use only with JPEG

  // check the esp32cam board has a psram chip installed (extra memory used for
  // storing captured images)
  //    Note: if not using "AI thinker esp32 cam" in the Arduino IDE, SPIFFS
  //    must be enabled
  if (!psramFound()) {
    Serial.println("Warning: No PSRam found so defaulting to image size 'CIF'");
    config.frame_size = FRAMESIZE_CIF;
  }

  esp_err_t camerr = esp_camera_init(&config);  // initialise the camera
  if (camerr != ESP_OK) {
    Serial.printf("ERROR: Camera init failed with error 0x%x", camerr);
  }

  cameraImageSettings();  // apply custom camera settings

  return (camerr == ESP_OK);  // return boolean result of camera initialisation
}

bool cameraImageSettings() {
  Serial.println("Applying camera settings");

  sensor_t *s = esp_camera_sensor_get();

  if (s == NULL) {
    Serial.println("Error: problem reading camera sensor settings");
    return 0;
  }

  // if both set to zero enable auto adjust
  if (cameraImageExposure == 0 && cameraImageGain == 0) {
    // enable auto adjust
    s->set_gain_ctrl(s, 1);      // auto gain on
    s->set_exposure_ctrl(s, 1);  // auto exposure on
    s->set_awb_gain(s, 1);       // Auto White Balance enable (0 or 1)
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
  } else {
    // Apply manual settings
    s->set_gain_ctrl(s, 0);      // auto gain off
    s->set_awb_gain(s, 1);       // Auto White Balance enable (0 or 1)
    s->set_exposure_ctrl(s, 0);  // auto exposure off
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
    s->set_agc_gain(s, cameraImageGain);          // set gain manually (0 - 30)
    s->set_aec_value(s, cameraImageExposure);  // set exposure manually (0-1200)
  }

  return 1;
}